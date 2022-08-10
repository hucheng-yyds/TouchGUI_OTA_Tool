#include "mainwindow.h"
#include <QApplication>
#include "log.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Log l;
    MainWindow w;
    w.show();
    return a.exec();
}
