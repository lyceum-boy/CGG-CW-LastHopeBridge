#include "glwidget.h"

#include <cstdlib>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    connect(&m_timer, &QTimer::timeout, this, &GLWidget::tick);
    m_timer.start(16); // ~60 FPS
    m_clock.start();
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearColor(0.55f, 0.75f, 0.95f, 1.0f);

    m_scene.init(this);
    m_lastMs = m_clock.elapsed();
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL()
{
    // Плавное затемнение фона вместе со сменой дня и ночи
    const float t = m_scene.dayNightFactor(); // 0 - день, 1 - ночь

    const float dayR = 0.55f, dayG = 0.75f, dayB = 0.95f;       // Дневное небо
    const float nightR = 0.05f, nightG = 0.07f, nightB = 0.12f; // Ночное небо

    const float r = dayR * (1.0f - t) + nightR * t;
    const float g = dayG * (1.0f - t) + nightG * t;
    const float b = dayB * (1.0f - t) + nightB * t;

    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_scene.draw(this, width(), height());
}

void GLWidget::tick()
{
    qint64 now = m_clock.elapsed();
    float dt = float(now - m_lastMs) / 1000.0f;
    m_lastMs = now;

    if (dt > 0.05f) dt = 0.05f; // Ограничение шага по времени

    applyInput(dt);
    m_scene.update(dt);

    update();
}

void GLWidget::applyInput(float dt)
{
    // Управление камерой:
    // ← / → : вращение
    // ↑ / ↓ : приближение / отдаление
    // A / D : смещение влево / вправо
    // W / S : наклон камеры вверх / вниз

    float rotSpeed = 1.2f;
    float zoomSpeed = 18.0f;
    float slideSpeed = 12.0f;
    float pitchSpeed = 1.0f;

    if (m_keys.contains(Qt::Key_Left))  m_scene.cam.rotate(-rotSpeed*dt, 0);
    if (m_keys.contains(Qt::Key_Right)) m_scene.cam.rotate(+rotSpeed*dt, 0);
    if (m_keys.contains(Qt::Key_Up))    m_scene.cam.zoom(-zoomSpeed*dt);
    if (m_keys.contains(Qt::Key_Down))  m_scene.cam.zoom(+zoomSpeed*dt);

    if (m_keys.contains(Qt::Key_A)) m_scene.cam.slide(-slideSpeed*dt);
    if (m_keys.contains(Qt::Key_D)) m_scene.cam.slide(+slideSpeed*dt);

    if (m_keys.contains(Qt::Key_W)) m_scene.cam.rotate(0, +pitchSpeed*dt);
    if (m_keys.contains(Qt::Key_S)) m_scene.cam.rotate(0, -pitchSpeed*dt);
}

void GLWidget::onKeyPress(QKeyEvent* e)
{
    if (e->isAutoRepeat()) return;
    if (e->key() == Qt::Key_Space){
        if (m_scene.canTriggerNight()) m_scene.triggerNight();
        return;
    }
    m_keys.insert(e->key());
    if (e->key() == Qt::Key_Escape) close();
}

void GLWidget::onKeyRelease(QKeyEvent* e)
{
    m_keys.remove(e->key());
}

void GLWidget::onMousePress(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton){
        m_mouseDown = true;
        m_lastMouse = e->pos();
        m_pressMouse = e->pos();
    }
}

void GLWidget::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton){
        m_mouseDown = false;

        // Считать кликом, если мышь почти не двигалась
        const QPoint rel = e->pos() - m_pressMouse;
        const int manhattan = std::abs(rel.x()) + std::abs(rel.y());
        if (manhattan <= 4){
            m_scene.handleClick(e->pos().x(), e->pos().y(), width(), height());
        }
    }
}

void GLWidget::onMouseMove(QMouseEvent* e)
{
    if (!m_mouseDown) return;
    QPoint d = e->pos() - m_lastMouse;
    m_lastMouse = e->pos();

    float sens = 0.005f;
    m_scene.cam.rotate(d.x()*sens, -d.y()*sens);
}

void GLWidget::onWheel(QWheelEvent* e)
{
    float num = e->angleDelta().y() / 120.0f;
    m_scene.cam.zoom(-num * 2.5f);
}
