#include "financialwidget.h"
#include "ui_financialwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QChartView>
#include <QtableWidget>
#include <QPushButton>
#include <QDateEdit>
#include <QLabel>
#include <QComboBox>
FinancialWidget::FinancialWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FinancialWidget)
{
    ui->setupUi(this);
    setupUI();
}

FinancialWidget::~FinancialWidget()
{
    delete ui;
}

void FinancialWidget::setupUI()
{
    //  1. 整体布局初始化
    // 创建主垂直布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 顶部水平布局（用于放置筛选条件和按钮）
    QHBoxLayout* topLayout = new QHBoxLayout();
    // 中部水平布局（用于并排显示表格和饼图）
    QHBoxLayout* middleLayyout = new QHBoxLayout();

    // 创建图表视图（用于底部的统计图表）
    chartView = new QChartView();

    // 将各个布局和控件加入主布局，并设置比例
    mainLayout->addLayout(topLayout);       // 顶部自适应高度
    mainLayout->addLayout(middleLayyout, 60); // 中部占整体高度的 60%
    mainLayout->addWidget(chartView, 40);   // 底部图表占整体高度的 40%

    //  2. 顶部筛选条件与按钮区
    topLayout->addWidget(new QLabel("医生姓名:", this));
    studentComboBox = new QComboBox(this);
    topLayout->addWidget(studentComboBox);

    topLayout->addWidget(new QLabel("起始日期:", this));
    // 默认起始日期为一个月前
    startDateEdit = new QDateEdit(QDate::currentDate().addMonths(-1));
    startDateEdit->setCalendarPopup(true); // 开启日历弹窗
    topLayout->addWidget(startDateEdit);

    topLayout->addWidget(new QLabel("结束日期:", this));
    // 默认结束日期为今天
    endDateEdit = new QDateEdit(QDate::currentDate());
    endDateEdit->setCalendarPopup(true);
    topLayout->addWidget(endDateEdit);

    // 创建三个操作按钮
    addButton = new QPushButton("添加");
    deleteButton = new QPushButton("删除");
    editButton = new QPushButton("修改");
    topLayout->addWidget(addButton);
    topLayout->addWidget(deleteButton);
    topLayout->addWidget(editButton);

    // 添加弹簧，把左边的筛选条件和按钮往左挤
    topLayout->addStretch();

    // 3. 中部区域：表格与饼图
    tableWidget = new QTableWidget();
    tableWidget->setFixedWidth(550); // 固定表格宽度，给饼图留出空间
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止直接在表格里编辑（需要点“修改”按钮）
    tableWidget->setAlternatingRowColors(true); // 开启斑马线交替行背景

    // 设置表头
    QStringList header = QStringList() << "ID" << "医生名字" << "工资日期" << "金额" << "支付类型" << "备注";
    tableWidget->setColumnCount(header.count());
    tableWidget->setHorizontalHeaderLabels(header);
    tableWidget->setColumnHidden(0, true); // 隐藏第 0 列（ID 列），因为数据库主键不需要给用户看

    middleLayyout->addWidget(tableWidget); // 表格放左边

    pieChartView = new QChartView();       // 创建饼图视图
    middleLayyout->addWidget(pieChartView); // 饼图放右边，平分剩余空间

    // 4. 底部图表设置
    chartView->setRenderHint(QPainter::Antialiasing); // 开启抗锯齿，让图表边缘更平滑
    chartView->setMinimumHeight(200);                 // 保证图表最小高度

    //  5. 信号与槽连接
    // 按钮点击事件
    // connect(addButton, &QPushButton::clicked, this, &FinancialWidget::addRecord);
    // connect(deleteButton, &QPushButton::clicked, this, &FinancialWidget::deleteRecord);
    // connect(editButton, &QPushButton::clicked, this, &FinancialWidget::editRecord);

    // // 筛选条件改变时，自动重新加载数据
    // // 使用 QOverload 消除 currentIndexChanged 的重载歧义
    // connect(studentComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
    //         this, &FinancialWidget::loadFinancialRecords);
    // connect(startDateEdit, &QDateEdit::dateChanged, this, &FinancialWidget::loadFinancialRecords);
    // connect(endDateEdit, &QDateEdit::dateChanged, this, &FinancialWidget::loadFinancialRecords);
}
