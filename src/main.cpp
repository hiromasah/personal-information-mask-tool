#include <QtGui/QApplication>
#include "mainwindow.h"
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    translator.load("PMP");
    a.installTranslator(&translator);

    MainWindow w;
    w.show();

    return a.exec();
}
