#include "transitioncontroller.h"

#include <QEasingCurve>
#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickView>
#include <QScreen>
#include <memory>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVariantAnimation>

namespace {

QUrl imageUrl(const QString &path)
{
    return QUrl::fromLocalFile(path);
}

} // namespace

TransitionController::TransitionController(QObject *parent)
    : QObject(parent)
    , m_animation(new QVariantAnimation(this))
{
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setDuration(m_durationMs);
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);
}

TransitionController::~TransitionController()
{
    clearViews();
}

void TransitionController::setDuration(int durationMs)
{
    m_durationMs = qMax(100, durationMs);
    m_animation->setDuration(m_durationMs);
}

int TransitionController::duration() const
{
    return m_durationMs;
}

bool TransitionController::isRunning() const
{
    return m_running;
}

void TransitionController::start(const QString &fromWallpaper, const QString &toWallpaper, const std::function<void()> &midpointAction)
{
    if (m_running) {
        return;
    }

    m_views = createViews(fromWallpaper, toWallpaper);
    if (m_views.isEmpty()) {
        midpointAction();
        emit finished();
        return;
    }

    m_running = true;
    auto midpointTriggered = std::make_shared<bool>(false);

    connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        const qreal progress = value.toReal();
        for (QQuickView *view : std::as_const(m_views)) {
            if (view->rootObject()) {
                view->rootObject()->setProperty("progress", progress);
            }
        }
    });

    connect(m_animation, &QVariantAnimation::finished, this, [this]() {
        clearViews();
        m_running = false;
        disconnect(m_animation, nullptr, this, nullptr);
        emit finished();
    });

    QTimer::singleShot(m_durationMs / 3, this, [midpointAction, midpointTriggered]() mutable {
        if (!*midpointTriggered) {
            *midpointTriggered = true;
            midpointAction();
        }
    });

    m_animation->stop();
    m_animation->start();
}

void TransitionController::clearViews()
{
    qDeleteAll(m_views);
    m_views.clear();
}

QList<QQuickView *> TransitionController::createViews(const QString &fromWallpaper, const QString &toWallpaper)
{
    QList<QQuickView *> views;
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return views;
    }

    for (QScreen *screen : screens) {
        auto *view = new QQuickView();
        view->setColor(Qt::transparent);
        view->setSource(QUrl(QStringLiteral("qrc:/qml/TransitionOverlay.qml")));
        view->setScreen(screen);
        view->setGeometry(screen->geometry());
        view->setResizeMode(QQuickView::SizeRootObjectToView);
        view->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        if (view->rootObject()) {
            view->rootObject()->setProperty("oldSource", imageUrl(fromWallpaper));
            view->rootObject()->setProperty("newSource", imageUrl(toWallpaper));
            view->rootObject()->setProperty("progress", 0.0);
        }
        view->showFullScreen();
        views.push_back(view);
    }

    return views;
}
