#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include "databasemangager.h"
#include <QFile>
#include "logindialog.h"
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

    loginDialog loginDlg;
    if(loginDlg.exec() == QDialog::Accepted){
        MainWindow w;
        w.show();
        return QApplication::exec();
    }

    return 0;

}
