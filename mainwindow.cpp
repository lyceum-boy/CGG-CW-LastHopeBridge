#include "mainwindow.h"
#include "aboutdialog.h"
#include "glwidget.h"

#include <QApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Мост «Последней надежды»");
    setMinimumSize(800, 600);
    resize(800, 600);

    QIcon appIcon(":/img/icon.png");
    if (appIcon.isNull()) {
        qDebug() << "Иконка не найдена в ресурсах";
    } else {
        setWindowIcon(appIcon);
        qDebug() << "Иконка успешно загружена из ресурсов";
    }

    m_gl = new GLWidget(this);
    setCentralWidget(m_gl);
    setFocusPolicy(Qt::StrongFocus);

    initAboutDialog();
}

void MainWindow::initAboutDialog()
{
    // Диалоговое окно создается один раз
    m_aboutDlg = new AboutDialog(this);
    m_aboutDlg->setModal(true);

    m_aboutShortcut = new QShortcut(QKeySequence("Ctrl+?"), this);
    m_aboutShortcut->setContext(Qt::ApplicationShortcut);

    connect(m_aboutShortcut, &QShortcut::activated, this, [this]() {
        if (!m_aboutDlg) return;

        // Отображение диалогового окна
        if (m_aboutDlg->isVisible()) {
            m_aboutDlg->raise();
            m_aboutDlg->activateWindow();
            return;
        }

        m_aboutDlg->setWindowIcon(QApplication::windowIcon());
        m_aboutDlg->show();
        m_aboutDlg->raise();
        m_aboutDlg->activateWindow();
    });
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (m_gl) m_gl->onKeyPress(e);
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    if (m_gl) m_gl->onKeyRelease(e);
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if (m_gl) m_gl->onMousePress(e);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_gl) m_gl->onMouseRelease(e);
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (m_gl) m_gl->onMouseMove(e);
}

void MainWindow::wheelEvent(QWheelEvent *e)
{
    if (m_gl) m_gl->onWheel(e);
}
