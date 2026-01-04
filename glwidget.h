#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QElapsedTimer>
#include <QTimer>
#include <QSet>

#include "scene/scene.h"

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit GLWidget(QWidget* parent = nullptr);
    ~GLWidget() override = default;

    // Обработчики событий окна
    void onKeyPress(QKeyEvent* e);
    void onKeyRelease(QKeyEvent* e);
    void onMousePress(QMouseEvent* e);
    void onMouseRelease(QMouseEvent* e);
    void onMouseMove(QMouseEvent* e);
    void onWheel(QWheelEvent* e);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private slots:
    void tick();

private:
    Scene m_scene;

    QTimer m_timer;
    QElapsedTimer m_clock;
    qint64 m_lastMs = 0;

    QSet<int> m_keys;

    bool m_mouseDown = false;
    QPoint m_lastMouse;
    QPoint m_pressMouse;

    void applyInput(float dt);
};

#endif // GLWIDGET_H
