#include "shortcutmanager.h"

#include <KGlobalAccel>

#include <QAction>
#include <QDebug>

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
    registerShortcut(action, shortcuts);
    return action;
}

void ShortcutManager::registerShortcut(QAction *action, const QList<QKeySequence> &shortcuts)
{
    KGlobalAccel::self()->setDefaultShortcut(action, shortcuts);
    const bool success = KGlobalAccel::self()->setShortcut(action, shortcuts, KGlobalAccel::Autoloading);
    const QList<QKeySequence> appliedShortcuts = KGlobalAccel::self()->shortcut(action);

    if (!success || appliedShortcuts != shortcuts) {
        qWarning() << "Global shortcut registration mismatch for" << action->objectName()
                   << "requested:" << shortcuts
                   << "applied:" << appliedShortcuts;
    }
}
