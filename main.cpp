#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    // 加这一句，测试文件是否存在
    qDebug() << "文件是否存在:" << QFile::exists(":/ico/416.bmp");
    return QApplication::exec();
}
