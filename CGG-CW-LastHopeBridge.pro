QT       += core gui opengl openglwidgets multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
TEMPLATE = app
TARGET = LastHopeBridge

win32 {
    RC_ICONS = assets/img/icon.ico
}

QMAKE_CXXFLAGS += -Wall -Wextra

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdialog.cpp \
    core/mesh.cpp \
    core/objloader.cpp \
    core/shader.cpp \
    core/texture.cpp \
    glwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    scene/boat.cpp \
    scene/bridge.cpp \
    scene/camera.cpp \
    scene/object.cpp \
    scene/scene.cpp \
    scene/vehicle.cpp

HEADERS += \
    aboutdialog.h \
    core/math3d.h \
    core/mesh.h \
    core/objloader.h \
    core/shader.h \
    core/texture.h \
    glwidget.h \
    mainwindow.h \
    scene/boat.h \
    scene/bridge.h \
    scene/camera.h \
    scene/object.h \
    scene/scene.h \
    scene/vehicle.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

FORMS += \
    aboutdialog.ui
