#include "shortcutmanager.h"

#include <KGlobalAccel>

#include <QAction>

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent)
{
    auto *nextWallpaperAction = createAction(
        tr("Next Wallpaper"),
        QStringLiteral("next-wallpaper"),
        {QKeySequence(QStringLiteral("Meta+F11"))});
    auto *randomWallpaperAction = createAction(
        tr("Random Wallpaper"),
        QStringLiteral("random-wallpaper"),
        {QKeySequence(QStringLiteral("Meta+F12"))});

    connect(nextWallpaperAction, &QAction::triggered, this, &ShortcutManager::nextWallpaperRequested);
    connect(randomWallpaperAction, &QAction::triggered, this, &ShortcutManager::randomWallpaperRequested);
}

QAction *ShortcutManager::createAction(const QString &name, const QString &componentName, const QList<QKeySequence> &shortcuts)
{
    auto *action = new QAction(name, this);
    action->setObjectName(componentName);
    KGlobalAccel::self()->setDefaultShortcut(action, shortcuts);
    KGlobalAccel::self()->setShortcut(action, shortcuts, KGlobalAccel::NoAutoloading);
    return action;
}
