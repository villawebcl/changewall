#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct Profile {
    QString name;
    QString wallpaper;
    QString wallpaperDir;
    QString colorScheme;
    QString lookAndFeel;

    bool isValid() const;
    bool isDark() const;
    QJsonObject toJson() const;
    static Profile fromJson(const QJsonObject &object);
};

class ProfileManager : public QObject {
    Q_OBJECT

public:
    explicit ProfileManager(QObject *parent = nullptr);

    bool load(const QString &configPath, QString *errorMessage = nullptr);
    bool saveState(QString *errorMessage = nullptr) const;
    bool loadState();

    QString configPath() const;
    QString statePath() const;
    QVector<Profile> profiles() const;
    QStringList profileNames() const;
    int currentIndex() const;
    Profile currentProfile() const;

    int indexOfProfile(const QString &name) const;
    Profile profileAt(int index) const;
    int nextIndex() const;
    int toggledIndex() const;
    int randomWallpaperSourceIndex() const;
    QStringList wallpaperListForIndex(int index) const;
    QString resolvedWallpaperForIndex(int index) const;
    QString randomWallpaperForIndex(int index, const QString &excludePath = {}) const;

    void setCurrentIndex(int index);
    void setCurrentWallpaperOverride(const QString &path);
    QString currentWallpaperPath() const;

signals:
    void profilesReloaded();

private:
    QString m_configPath;
    QString m_statePath;
    QVector<Profile> m_profiles;
    int m_currentIndex = -1;
    QString m_currentWallpaperOverride;
};
