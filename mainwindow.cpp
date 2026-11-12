#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QButtonGroup>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //把左边的按钮关联成一个组,一个组里面只有一个按钮可以被选中
    QButtonGroup *btnGp = new QButtonGroup(this);  //采用指针是因为Qt有自动内存管理的功能
    btnGp->addButton(ui->btDoctorInfo, 0);
    btnGp->addButton(ui->btSchedule, 1);
    btnGp->addButton(ui->btFinance, 2);
    btnGp->addButton(ui->btHonor, 3);
    btnGp->addButton(ui->btSystemSetting, 4);
    //按钮关联stackwidget界面
    connect(btnGp, &QButtonGroup::idClicked, ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    btnGp->button(0)->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}
