#pragma once

#include <QObject>
#include <QString>

class PlasmaIntegration : public QObject {
    Q_OBJECT

public:
    explicit PlasmaIntegration(QObject *parent = nullptr);

    bool applyWallpaper(const QString &wallpaperPath, QString *errorMessage = nullptr) const;

private:
    QString wallpaperScript(const QString &wallpaperPath) const;
};
