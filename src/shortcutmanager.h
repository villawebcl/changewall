#pragma once

#include <QObject>

class QAction;

class ShortcutManager : public QObject {
    Q_OBJECT

public:
    explicit ShortcutManager(QObject *parent = nullptr);

signals:
    void nextWallpaperRequested();
    void randomWallpaperRequested();

private:
    QAction *createAction(const QString &name, const QString &componentName, const QList<QKeySequence> &shortcuts);
};
