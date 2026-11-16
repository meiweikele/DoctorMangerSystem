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
#include <QSqlQuery>
#include <QHeaderView>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSqlError>
#include <QLineEdit>
FinancialWidget::FinancialWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FinancialWidget)
{
    ui->setupUi(this);
    setupUI();
    populateDoctorComboBox();
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
    DoctorComboBox = new QComboBox(this);
    topLayout->addWidget(DoctorComboBox);

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
    connect(addButton, &QPushButton::clicked, this, &FinancialWidget::addRecord);
    // connect(deleteButton, &QPushButton::clicked, this, &FinancialWidget::deleteRecord);
    // connect(editButton, &QPushButton::clicked, this, &FinancialWidget::editRecord);

    // 筛选条件改变时，自动重新加载数据
    // 使用 QOverload 消除 currentIndexChanged 的重载歧义
    connect(DoctorComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FinancialWidget::loadFinancialRecords);
    connect(startDateEdit, &QDateEdit::dateChanged, this, &FinancialWidget::loadFinancialRecords);
    connect(endDateEdit, &QDateEdit::dateChanged, this, &FinancialWidget::loadFinancialRecords);
}

void FinancialWidget::loadFinancialRecords()
{
    //  1. 清空表格旧数据
    tableWidget->setRowCount(0);

    // 2. 获取筛选条件
    // 从下拉框获取当前选中医生的 ID（UserRole 隐藏数据）
    QString DoctorId = DoctorComboBox->currentData().toString();
    // 获取起始和结束日期
    QDate startDate = startDateEdit->date();
    QDate endDate = endDateEdit->date();

    //  3. 动态拼接 SQL 查询语句
    // 这里使用了 QString 的 arg 链式拼接，巧妙的点在于 %3 的位置：
    // 如果选中了具体医生（DoctorId != "-1"），就拼上 "AND fr._id = 'xxx'"
    // 如果选中的是全部医生（通常下拉框第一项数据值为 -1），就拼上空字符串
    QString queryStr = QString(
                           "SELECT fr.id, s.name, fr.payment_date, fr.amount, fr.payment_type, fr.notes "
                           "FROM financialRecords fr "
                           "JOIN DoctorInfo s ON fr.Doctor_id = s.id " // 连接医生表获取医生姓名
                           "WHERE fr.payment_date BETWEEN '%1' AND '%2' %3"
                           ).arg(startDate.toString("yyyy-MM-dd"),
                                endDate.toString("yyyy-MM-dd"),
                                (DoctorId != "-1") ? QString("AND fr._id = '%1'").arg(DoctorId) : "");

    //  4. 执行查询并填充表格
    QSqlQuery query(queryStr);

    // 遍历查询结果集的每一行
    while (query.next()) {
        int row = tableWidget->rowCount(); // 获取当前行数作为新行索引
        tableWidget->insertRow(row);       // 插入新行

        // 循环填充这一行的 6 列数据
        for (int col = 0; col < 6; ++col) {
            // 从查询结果中取出第 col 个字段，创建表格项
            QTableWidgetItem* item = new QTableWidgetItem(query.value(col).toString());
            item->setTextAlignment(Qt::AlignCenter); // 文字居中
            tableWidget->setItem(row, col, item);    // 放入单元格
        }
    }

    //  5. 设置表头对齐并刷新图表
    tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter); // 表头文字居中

    updateChart();     // 根据最新数据更新底部统计图表
    updatePieChart();  // 根据最新数据更新右侧饼状图
}

void FinancialWidget::updateChart()
{

}

void FinancialWidget::updatePieChart()
{

}
//这块有点问题，界面不刷新
void FinancialWidget::populateDoctorComboBox()
{
    //  1. 清空旧数据
    // 每次重新加载或刷新时，先清空下拉框原有的选项，防止重复添加
    DoctorComboBox->clear();

    // 2. 添加“所有医生”默认选项
    // 显示文本为“所有医生”，实际隐藏数据（UserRole）绑定为字符串 "-1"
    // 后续在查询时，通过判断数据是否为 "-1" 来决定是否加上 WHERE 医生 id = xxx 的过滤条件
    DoctorComboBox->addItem("所有医生", QVariant("-1"));

    // = 3. 从数据库读取医生列表
    QSqlQuery query("SELECT id, name FROM DoctorInfo");

    // 遍历查询结果集
    while (query.next()) {
        // 取出医生 ID 和姓名（这里 ID 是 TEXT 字符串类型）
        QString id = query.value(0).toString();
        QString name = query.value(1).toString();

        // 将医生姓名显示在下拉框中，把真实的 ID 作为隐藏数据绑定在后面
        DoctorComboBox->addItem(name, QVariant(id));
    }
}

void FinancialWidget::addRecord()
{
    QDialog dialog(this);
    dialog.setWindowTitle("添加缴费记录");
    QFormLayout form(&dialog);
    // 学生名称下拉菜单
    QComboBox* DoctorNameComboBox = new QComboBox(&dialog);
    QSqlQuery query("SELECT id, name FROM DoctorInfo");
    while (query.next()) {
        QString id = query.value(0).toString();
        QString name = query.value(1).toString();
        DoctorNameComboBox->addItem(name, QVariant(id)); // 将Doctor ID与名称关联
    }
    QDateEdit* paymentDateEdit = new QDateEdit(&dialog);
    paymentDateEdit->setDate(QDate::currentDate()); // 设置默认值为当前日期
    paymentDateEdit->setCalendarPopup(true); // 允许弹出日历选择器

    QLineEdit* amountEdit = new QLineEdit(&dialog);
    QLineEdit* feeTypeEdit = new QLineEdit(&dialog);
    QLineEdit* remarkEdit = new QLineEdit(&dialog);

    form.addRow("医生名称:", DoctorNameComboBox);
    form.addRow("缴费日期:", paymentDateEdit); // 修改为 QDateEdit
    form.addRow("金额:", amountEdit);
    form.addRow("支付类型:", feeTypeEdit);
    form.addRow("备注:", remarkEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    buttonBox.button(QDialogButtonBox::Ok)->setText("确定");
    buttonBox.button(QDialogButtonBox::Cancel)->setText("取消");
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString DoctorId = DoctorNameComboBox->currentData().toString();
        QString paymentDate = paymentDateEdit->date().toString("yyyy-MM-dd");
        double amount = amountEdit->text().toDouble();
        QString feeType = feeTypeEdit->text();
        QString remark = remarkEdit->text();
        // 准备SQL查询
        QSqlQuery query;
        query.prepare("INSERT INTO financialRecords (doctor_id, payment_date, amount, payment_type, notes) "
                      "VALUES (:doctor_id, :payment_date, :amount, :payment_type, :notes)");
        query.bindValue(":doctor_id", DoctorId); // 绑定学生ID
        query.bindValue(":payment_date", paymentDate);
        query.bindValue(":amount", amount);
        query.bindValue(":payment_type", feeType);
        query.bindValue(":notes", remark);
        // 执行SQL查询
        if (query.exec()) {
            qDebug() << "记录添加成功！";
            loadFinancialRecords(); // 刷新表格
        }
        else qDebug() << "添加记录失败：" << query.lastError().text();
    }
}


