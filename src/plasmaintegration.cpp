#include "plasmaintegration.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QFileInfo>

PlasmaIntegration::PlasmaIntegration(QObject *parent)
    : QObject(parent)
{
}

bool PlasmaIntegration::applyWallpaper(const QString &wallpaperPath, QString *errorMessage) const
{
    const QString absolutePath = QFileInfo(wallpaperPath).absoluteFilePath();
    if (!QFileInfo::exists(absolutePath)) {
        if (errorMessage) {
            *errorMessage = tr("Wallpaper does not exist: %1").arg(absolutePath);
        }
        return false;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.kde.plasmashell"),
        QStringLiteral("/PlasmaShell"),
        QStringLiteral("org.kde.PlasmaShell"),
        QStringLiteral("evaluateScript"));

    message << wallpaperScript(absolutePath);
    const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        if (errorMessage) {
            *errorMessage = reply.errorMessage();
        }
        return false;
    }

    return true;
}

QString PlasmaIntegration::wallpaperScript(const QString &wallpaperPath) const
{
    const QString escapedPath = QString(wallpaperPath).replace(QStringLiteral("\\"), QStringLiteral("\\\\"))
                                    .replace(QStringLiteral("\""), QStringLiteral("\\\""));

    return QStringLiteral(R"JS(
const wallpaper = "file://%1";
for (const desktop of desktops()) {
    desktop.wallpaperPlugin = "org.kde.image";
    desktop.currentConfigGroup = ["Wallpaper", "org.kde.image", "General"];
    desktop.writeConfig("Image", wallpaper);
}
)JS")
        .arg(escapedPath);
}
