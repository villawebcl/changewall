#include "commandserver.h"
#include "plasmaintegration.h"
#include "profilemanager.h"
#include "shortcutmanager.h"
#include "transitioncontroller.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>

namespace {

QString defaultConfigPath()
{
    const QString configRoot = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    const QString configDir = QDir(configRoot).filePath(QStringLiteral("changewall"));
    QDir().mkpath(configDir);
    return QDir(configDir).filePath(QStringLiteral("config.json"));
}

QString serverName()
{
    return QStringLiteral("changewall-daemon");
}

QString legacyConfigPath()
{
    const QString configRoot = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return QDir(configRoot).filePath(QStringLiteral("changewall/profiles.json"));
}

bool writeWallpaperConfig(const QString &configPath, const QString &wallpaperDir, QString *errorMessage)
{
    QDir().mkpath(QFileInfo(configPath).absolutePath());

    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Unable to write config file: %1").arg(configPath);
        }
        return false;
    }

    QJsonObject object;
    object.insert(QStringLiteral("wallpaperDir"), wallpaperDir);
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return true;
}

QString ensureConfigFile(const QString &requestedPath, bool interactive, QString *errorMessage)
{
    const QString normalizedPath = QFileInfo(requestedPath).absoluteFilePath();
    if (QFileInfo::exists(normalizedPath)) {
        return normalizedPath;
    }

    if (normalizedPath == QFileInfo(defaultConfigPath()).absoluteFilePath() && QFileInfo::exists(legacyConfigPath())) {
        return QFileInfo(legacyConfigPath()).absoluteFilePath();
    }

    if (!interactive) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Config file not found: %1").arg(normalizedPath);
        }
        return {};
    }

    const QString selectedDirectory = QFileDialog::getExistingDirectory(
        nullptr,
        QObject::tr("Select Wallpaper Folder"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (selectedDirectory.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("No wallpaper folder was selected.");
        }
        return {};
    }

    QString writeError;
    if (!writeWallpaperConfig(normalizedPath, selectedDirectory, &writeError)) {
        if (errorMessage) {
            *errorMessage = writeError;
        }
        return {};
    }

    QMessageBox::information(
        nullptr,
        QObject::tr("ChangeWall Ready"),
        QObject::tr("Wallpaper folder saved to %1").arg(normalizedPath));

    return normalizedPath;
}

QString runSetupDialog(const QString &requestedPath, QString *errorMessage)
{
    const QString normalizedPath = QFileInfo(requestedPath).absoluteFilePath();
    const QString selectedDirectory = QFileDialog::getExistingDirectory(
        nullptr,
        QObject::tr("Select Wallpaper Folder"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (selectedDirectory.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("No wallpaper folder was selected.");
        }
        return {};
    }

    QString writeError;
    if (!writeWallpaperConfig(normalizedPath, selectedDirectory, &writeError)) {
        if (errorMessage) {
            *errorMessage = writeError;
        }
        return {};
    }

    QMessageBox::information(
        nullptr,
        QObject::tr("ChangeWall Ready"),
        QObject::tr("Wallpaper folder saved to %1").arg(normalizedPath));

    return normalizedPath;
}

enum class CommandType {
    None,
    NextWallpaper,
    RandomWallpaper,
};

struct CommandRequest {
    CommandType type = CommandType::None;
};

CommandRequest parseCommandLine(const QCommandLineParser &parser)
{
    if (parser.isSet(QStringLiteral("next-wallpaper"))) {
        return {CommandType::NextWallpaper};
    }
    if (parser.isSet(QStringLiteral("random-wallpaper"))) {
        return {CommandType::RandomWallpaper};
    }
    return {};
}

QStringList serializeCommand(const CommandRequest &request)
{
    switch (request.type) {
    case CommandType::NextWallpaper:
        return {QStringLiteral("next-wallpaper")};
    case CommandType::RandomWallpaper:
        return {QStringLiteral("random-wallpaper")};
    case CommandType::None:
        return {};
    }
    return {};
}

class ApplicationController : public QObject {
    Q_OBJECT

public:
    ApplicationController(ProfileManager *profileManager,
                          PlasmaIntegration *plasmaIntegration,
                          TransitionController *transitionController,
                          QObject *parent = nullptr)
        : QObject(parent)
        , m_profileManager(profileManager)
        , m_plasmaIntegration(plasmaIntegration)
        , m_transitionController(transitionController)
    {
        connect(m_transitionController, &TransitionController::finished, this, &ApplicationController::finalizeIfIdle);
    }

    bool execute(const CommandRequest &request)
    {
        if (request.type == CommandType::None) {
            return true;
        }

        if (m_busy) {
            m_pendingCommand = request;
            return true;
        }

        switch (request.type) {
        case CommandType::NextWallpaper:
            return nextWallpaper();
        case CommandType::RandomWallpaper:
            return randomWallpaper();
        case CommandType::None:
            return true;
        }

        return false;
    }

private:
    bool randomWallpaper()
    {
        const int sourceIndex = m_profileManager->randomWallpaperSourceIndex();
        const QString currentWallpaper = m_profileManager->currentWallpaperPath();
        const QString wallpaper = m_profileManager->randomWallpaperForIndex(sourceIndex, currentWallpaper);
        if (wallpaper.isEmpty()) {
            qWarning() << "Unable to choose a random wallpaper";
            return false;
        }

        const QString fromWallpaper = currentWallpaper.isEmpty()
            ? m_profileManager->resolvedWallpaperForIndex(sourceIndex)
            : currentWallpaper;

        m_busy = true;
        m_transitionController->start(fromWallpaper, wallpaper, [this, sourceIndex, wallpaper]() {
            QString errorMessage;
            if (!m_plasmaIntegration->applyWallpaper(wallpaper, &errorMessage)) {
                qWarning() << "Wallpaper apply failed:" << errorMessage;
            }

            m_profileManager->setCurrentIndex(sourceIndex);
            m_profileManager->setCurrentWallpaperOverride(wallpaper);
        });

        return true;
    }

    bool nextWallpaper()
    {
        const int sourceIndex = m_profileManager->randomWallpaperSourceIndex();
        const QStringList wallpapers = m_profileManager->wallpaperListForIndex(sourceIndex);
        if (wallpapers.isEmpty()) {
            qWarning() << "No wallpapers found in the configured directory";
            return false;
        }

        const QString currentWallpaper = m_profileManager->currentWallpaperPath();
        int currentPos = wallpapers.indexOf(QFileInfo(currentWallpaper).absoluteFilePath());
        if (currentPos < 0) {
            currentPos = 0;
        }

        const QString nextWallpaper = wallpapers.at((currentPos + 1) % wallpapers.size());
        const QString fromWallpaper = currentWallpaper.isEmpty()
            ? m_profileManager->resolvedWallpaperForIndex(sourceIndex)
            : currentWallpaper;

        m_busy = true;
        m_transitionController->start(fromWallpaper, nextWallpaper, [this, sourceIndex, nextWallpaper]() {
            QString errorMessage;
            if (!m_plasmaIntegration->applyWallpaper(nextWallpaper, &errorMessage)) {
                qWarning() << "Wallpaper apply failed:" << errorMessage;
            }

            m_profileManager->setCurrentIndex(sourceIndex);
            m_profileManager->setCurrentWallpaperOverride(nextWallpaper);
        });

        return true;
    }

    ProfileManager *m_profileManager = nullptr;
    PlasmaIntegration *m_plasmaIntegration = nullptr;
    TransitionController *m_transitionController = nullptr;
    bool m_busy = false;
    CommandRequest m_pendingCommand;

    void finalizeIfIdle()
    {
        if (!m_busy) {
            return;
        }

        m_busy = false;
        m_profileManager->saveState();
        if (m_pendingCommand.type != CommandType::None) {
            const CommandRequest request = m_pendingCommand;
            m_pendingCommand = {};
            execute(request);
        }
    }
};

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("changewall"));
    app.setApplicationDisplayName(QStringLiteral("ChangeWall"));
    app.setOrganizationName(QStringLiteral("changewall"));
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("KDE Plasma wallpaper switcher with animated transitions"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({QStringList{QStringLiteral("c"), QStringLiteral("config")},
                      QStringLiteral("Path to config JSON file."),
                      QStringLiteral("path"),
                      defaultConfigPath()});
    parser.addOption({QStringLiteral("setup"), QStringLiteral("Open the wallpaper folder setup dialog and rewrite the config.")});
    parser.addOption({QStringLiteral("next-wallpaper"), QStringLiteral("Apply the next wallpaper from the active or configured wallpaper directory.")});
    parser.addOption({QStringLiteral("random-wallpaper"), QStringLiteral("Apply a random wallpaper from the active or configured wallpaper directory.")});
    parser.addOption({QStringLiteral("duration"), QStringLiteral("Transition duration in milliseconds."), QStringLiteral("ms"), QStringLiteral("420")});
    parser.process(app);

    const bool interactiveSetup = !app.arguments().contains(QStringLiteral("--next-wallpaper"))
        && !app.arguments().contains(QStringLiteral("--random-wallpaper"));
    QString errorMessage;
    QString effectiveConfigPath;

    if (parser.isSet(QStringLiteral("setup"))) {
        effectiveConfigPath = runSetupDialog(parser.value(QStringLiteral("config")), &errorMessage);
    } else {
        effectiveConfigPath = ensureConfigFile(parser.value(QStringLiteral("config")), interactiveSetup, &errorMessage);
    }

    if (effectiveConfigPath.isEmpty()) {
        qCritical() << errorMessage;
        return 1;
    }

    const CommandRequest requestedCommand = parseCommandLine(parser);
    if (requestedCommand.type != CommandType::None) {
        if (CommandServer::sendCommand(serverName(), serializeCommand(requestedCommand), &errorMessage)) {
            return 0;
        }
    }

    ProfileManager profileManager;
    if (!profileManager.load(effectiveConfigPath, &errorMessage)) {
        if (interactiveSetup) {
            effectiveConfigPath = runSetupDialog(parser.value(QStringLiteral("config")), &errorMessage);
            if (effectiveConfigPath.isEmpty() || !profileManager.load(effectiveConfigPath, &errorMessage)) {
                qCritical() << errorMessage;
                return 1;
            }
        } else {
            qCritical() << errorMessage;
            return 1;
        }
    }
    profileManager.loadState();

    PlasmaIntegration plasmaIntegration;
    TransitionController transitionController;
    transitionController.setDuration(parser.value(QStringLiteral("duration")).toInt());

    ShortcutManager shortcutManager;
    CommandServer commandServer;
    if (!commandServer.listen(serverName(), &errorMessage)) {
        qCritical() << "Unable to start command server:" << errorMessage;
        return 1;
    }

    ApplicationController controller(&profileManager, &plasmaIntegration, &transitionController);

    QObject::connect(&shortcutManager, &ShortcutManager::nextWallpaperRequested, &controller, [&controller]() {
        controller.execute({CommandType::NextWallpaper});
    });
    QObject::connect(&shortcutManager, &ShortcutManager::randomWallpaperRequested, &controller, [&controller]() {
        controller.execute({CommandType::RandomWallpaper});
    });
    QObject::connect(&commandServer, &CommandServer::commandReceived, &controller, [&controller](const QStringList &arguments) {
        if (arguments.isEmpty()) {
            return;
        }

        if (arguments.first() == QStringLiteral("next-wallpaper")) {
            controller.execute({CommandType::NextWallpaper});
        } else if (arguments.first() == QStringLiteral("random-wallpaper")) {
            controller.execute({CommandType::RandomWallpaper});
        }
    });

    if (requestedCommand.type != CommandType::None) {
        QTimer::singleShot(0, &controller, [&controller, requestedCommand]() {
            controller.execute(requestedCommand);
        });
    }

    return app.exec();
}

#include "main.moc"
