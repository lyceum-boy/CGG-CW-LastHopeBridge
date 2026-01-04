#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QIcon appIcon(":/img/icon.png");
    a.setApplicationName("LastHopeBridge");
    a.setApplicationDisplayName("Володарский мост");
    a.setWindowIcon(appIcon);
    MainWindow w;
    w.setWindowIcon(appIcon);
    w.show();
    return a.exec();
}
