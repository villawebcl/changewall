#include "profilemanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRandomGenerator>
#include <QStandardPaths>

namespace {

QString ensureAppDataPath(const QString &leaf)
{
    const QString configRoot = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    const QString base = QDir(configRoot).filePath(QStringLiteral("changewall"));
    QDir dir(base);
    dir.mkpath(QStringLiteral("."));
    return dir.filePath(leaf);
}

QString expandUserPath(QString path)
{
    if (path.startsWith(QStringLiteral("~/"))) {
        path.replace(0, 1, QDir::homePath());
    }
    return QDir::cleanPath(path);
}

QStringList wallpaperFilters()
{
    return {QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.png"),
            QStringLiteral("*.webp"), QStringLiteral("*.bmp")};
}

QFileInfoList availableWallpapers(const QString &directoryPath)
{
    QDir directory(directoryPath);
    return directory.entryInfoList(
        wallpaperFilters(),
        QDir::Files | QDir::Readable,
        QDir::Name);
}

} // namespace

bool Profile::isValid() const
{
    return !name.trimmed().isEmpty()
        && (!wallpaper.trimmed().isEmpty() || !wallpaperDir.trimmed().isEmpty());
}

bool Profile::isDark() const
{
    const QStringList tokens = {
        name.toLower(),
        colorScheme.toLower(),
        lookAndFeel.toLower(),
    };

    for (const QString &token : tokens) {
        if (token.contains(QStringLiteral("dark"))) {
            return true;
        }
    }

    return false;
}

QJsonObject Profile::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("name"), name);
    object.insert(QStringLiteral("wallpaper"), wallpaper);
    object.insert(QStringLiteral("wallpaperDir"), wallpaperDir);
    object.insert(QStringLiteral("colorscheme"), colorScheme);
    object.insert(QStringLiteral("lookandfeel"), lookAndFeel);
    return object;
}

Profile Profile::fromJson(const QJsonObject &object)
{
    Profile profile;
    profile.name = object.value(QStringLiteral("name")).toString();
    profile.wallpaper = expandUserPath(object.value(QStringLiteral("wallpaper")).toString());
    profile.wallpaperDir = expandUserPath(object.value(QStringLiteral("wallpaperDir")).toString());
    profile.colorScheme = object.value(QStringLiteral("colorscheme")).toString();
    profile.lookAndFeel = object.value(QStringLiteral("lookandfeel")).toString();
    return profile;
}

ProfileManager::ProfileManager(QObject *parent)
    : QObject(parent)
    , m_statePath(ensureAppDataPath(QStringLiteral("state.json")))
{
}

bool ProfileManager::load(const QString &configPath, QString *errorMessage)
{
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = tr("Unable to open config file: %1").arg(configPath);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = tr("Invalid config JSON: %1").arg(parseError.errorString());
        }
        return false;
    }

    QVector<Profile> loadedProfiles;
    const QJsonObject rootObject = document.object();
    const QJsonArray profilesArray = rootObject.value(QStringLiteral("profiles")).toArray();
    for (const QJsonValue &value : profilesArray) {
        if (!value.isObject()) {
            continue;
        }

        const Profile profile = Profile::fromJson(value.toObject());
        if (profile.isValid()) {
            loadedProfiles.push_back(profile);
        }
    }

    if (loadedProfiles.isEmpty()) {
        Profile profile;
        profile.name = QStringLiteral("default");
        profile.wallpaper = expandUserPath(rootObject.value(QStringLiteral("wallpaper")).toString());
        profile.wallpaperDir = expandUserPath(rootObject.value(QStringLiteral("wallpaperDir")).toString());
        profile.colorScheme = rootObject.value(QStringLiteral("colorscheme")).toString();
        profile.lookAndFeel = rootObject.value(QStringLiteral("lookandfeel")).toString();
        if (profile.isValid()) {
            loadedProfiles.push_back(profile);
        }
    }

    if (loadedProfiles.isEmpty()) {
        if (errorMessage) {
            *errorMessage = tr("No valid profiles found in %1").arg(configPath);
        }
        return false;
    }

    m_configPath = QFileInfo(configPath).absoluteFilePath();
    m_profiles = loadedProfiles;
    if (m_currentIndex < 0 || m_currentIndex >= m_profiles.size()) {
        m_currentIndex = 0;
    }

    emit profilesReloaded();
    return true;
}

bool ProfileManager::saveState(QString *errorMessage) const
{
    if (m_profiles.isEmpty()) {
        return true;
    }

    QFile file(m_statePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = tr("Unable to write state file: %1").arg(m_statePath);
        }
        return false;
    }

    QJsonObject object;
    object.insert(QStringLiteral("currentProfile"), currentProfile().name);
    object.insert(QStringLiteral("currentWallpaperOverride"), m_currentWallpaperOverride);
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return true;
}

bool ProfileManager::loadState()
{
    QFile file(m_statePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return false;
    }

    const QJsonObject object = document.object();
    const QString currentProfileName = object.value(QStringLiteral("currentProfile")).toString();
    const int index = indexOfProfile(currentProfileName);
    if (index >= 0) {
        m_currentIndex = index;
    }

    m_currentWallpaperOverride = object.value(QStringLiteral("currentWallpaperOverride")).toString();
    return true;
}

QString ProfileManager::configPath() const
{
    return m_configPath;
}

QString ProfileManager::statePath() const
{
    return m_statePath;
}

QVector<Profile> ProfileManager::profiles() const
{
    return m_profiles;
}

QStringList ProfileManager::profileNames() const
{
    QStringList names;
    names.reserve(m_profiles.size());
    for (const Profile &profile : m_profiles) {
        names.push_back(profile.name);
    }
    return names;
}

int ProfileManager::currentIndex() const
{
    return m_currentIndex;
}

Profile ProfileManager::currentProfile() const
{
    return profileAt(m_currentIndex);
}

int ProfileManager::indexOfProfile(const QString &name) const
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles.at(i).name.compare(name, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }
    return -1;
}

Profile ProfileManager::profileAt(int index) const
{
    if (index < 0 || index >= m_profiles.size()) {
        return {};
    }
    return m_profiles.at(index);
}

int ProfileManager::nextIndex() const
{
    if (m_profiles.isEmpty()) {
        return -1;
    }

    if (m_currentIndex < 0) {
        return 0;
    }

    return (m_currentIndex + 1) % m_profiles.size();
}

int ProfileManager::toggledIndex() const
{
    if (m_profiles.isEmpty()) {
        return -1;
    }

    const bool currentlyDark = currentProfile().isDark();
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (i == m_currentIndex) {
            continue;
        }

        if (m_profiles.at(i).isDark() != currentlyDark) {
            return i;
        }
    }

    return nextIndex();
}

int ProfileManager::randomWallpaperSourceIndex() const
{
    if (m_profiles.isEmpty()) {
        return -1;
    }

    if (m_currentIndex >= 0 && !m_profiles.at(m_currentIndex).wallpaperDir.isEmpty()) {
        return m_currentIndex;
    }

    for (int i = 0; i < m_profiles.size(); ++i) {
        if (!m_profiles.at(i).wallpaperDir.isEmpty()) {
            return i;
        }
    }

    return m_currentIndex >= 0 ? m_currentIndex : 0;
}

QStringList ProfileManager::wallpaperListForIndex(int index) const
{
    const Profile profile = profileAt(index);
    if (!profile.isValid() || profile.wallpaperDir.isEmpty()) {
        return {};
    }

    const QFileInfoList entries = availableWallpapers(profile.wallpaperDir);
    QStringList paths;
    paths.reserve(entries.size());
    for (const QFileInfo &entry : entries) {
        paths.push_back(entry.absoluteFilePath());
    }
    return paths;
}

QString ProfileManager::randomWallpaperForIndex(int index, const QString &excludePath) const
{
    const Profile profile = profileAt(index);
    if (!profile.isValid()) {
        return {};
    }

    if (!profile.wallpaperDir.isEmpty()) {
        const QFileInfoList entries = availableWallpapers(profile.wallpaperDir);

        if (!entries.isEmpty()) {
            QVector<QString> candidates;
            candidates.reserve(entries.size());
            const QString normalizedExclude = QFileInfo(excludePath).absoluteFilePath();

            for (const QFileInfo &entry : entries) {
                const QString path = entry.absoluteFilePath();
                if (!normalizedExclude.isEmpty() && path == normalizedExclude && entries.size() > 1) {
                    continue;
                }
                candidates.push_back(path);
            }

            if (!candidates.isEmpty()) {
                const int selection = QRandomGenerator::global()->bounded(candidates.size());
                return candidates.at(selection);
            }
        }
    }

    return resolvedWallpaperForIndex(index);
}

QString ProfileManager::resolvedWallpaperForIndex(int index) const
{
    const Profile profile = profileAt(index);
    if (!profile.isValid()) {
        return {};
    }

    if (!profile.wallpaper.trimmed().isEmpty() && QFileInfo::exists(profile.wallpaper)) {
        return QFileInfo(profile.wallpaper).absoluteFilePath();
    }

    if (!profile.wallpaperDir.trimmed().isEmpty()) {
        const QFileInfoList entries = availableWallpapers(profile.wallpaperDir);
        if (!entries.isEmpty()) {
            return entries.first().absoluteFilePath();
        }
    }

    return {};
}

QString ProfileManager::currentWallpaperPath() const
{
    if (!m_currentWallpaperOverride.isEmpty()) {
        return m_currentWallpaperOverride;
    }

    const QString resolved = resolvedWallpaperForIndex(m_currentIndex);
    if (!resolved.isEmpty()) {
        return resolved;
    }

    return currentProfile().wallpaper;
}

void ProfileManager::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_profiles.size()) {
        return;
    }

    m_currentIndex = index;
}

void ProfileManager::setCurrentWallpaperOverride(const QString &path)
{
    m_currentWallpaperOverride = path;
}
