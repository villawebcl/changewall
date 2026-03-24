#pragma once

#include <QObject>
#include <QString>

class QVariantAnimation;
class QQuickView;
class QScreen;

class TransitionController : public QObject {
    Q_OBJECT

public:
    explicit TransitionController(QObject *parent = nullptr);
    ~TransitionController() override;

    void setDuration(int durationMs);
    int duration() const;
    bool isRunning() const;

    void start(const QString &fromWallpaper, const QString &toWallpaper, const std::function<void()> &midpointAction);

signals:
    void finished();

private:
    void clearViews();
    QList<QQuickView *> createViews(const QString &fromWallpaper, const QString &toWallpaper);

    int m_durationMs = 420;
    QVariantAnimation *m_animation = nullptr;
    QList<QQuickView *> m_views;
    bool m_running = false;
};
