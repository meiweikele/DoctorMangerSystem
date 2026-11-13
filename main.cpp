#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include "databasemangager.h"
#include <QFile>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DataBaseMangager ::instance();
    QFile styleFile(":/style/style.qss");
    if(styleFile.open(QFile::ReadOnly))
    {
        QString styleSheet = QString(styleFile.readAll());
        a.setStyleSheet(styleSheet);  //应用样式表
        styleFile.close();
    }
    else
    {
        qWarning() << "failed to load styleSheet：" << styleFile.errorString();
    }

    MainWindow w;
    w.show();

    // 加这一句，测试文件是否存在
    qDebug() << "文件是否存在:" << QFile::exists(":/ico/416.bmp");
    return QApplication::exec();
}
