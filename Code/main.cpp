#include "mainwindow.h"
#include <QStandardPaths>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    w.setFixedHeight(880);
    w.setFixedWidth(1111);
    return a.exec();
}
