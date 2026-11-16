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
#include <QPieSeries>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
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
    //connect(deleteButton, &QPushButton::clicked, this, &FinancialWidget::deleteRecord);
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
                           "JOIN DoctorInfo s ON fr.doctor_id = s.id "
                           "WHERE fr.payment_date BETWEEN '%1' AND '%2' %3"
                           ).arg(startDate.toString("yyyy-MM-dd"),
                                endDate.toString("yyyy-MM-dd"),
                                (DoctorId != "-1") ? QString("AND fr.doctor_id = '%1'").arg(DoctorId) : "");

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
    // ================== 1. 获取并验证日期范围 ==================
    QDate startDate = startDateEdit->date();
    QDate endDate = endDateEdit->date();
    // 如果用户把起始日期设得比结束日期还晚，代码会强行交换这两个日期，并重新设置回 UI
    if (startDate > endDate) {
        std::swap(startDate, endDate);
        startDateEdit->setDate(startDate);
        endDateEdit->setDate(endDate);
    }

    // ================== 2. 构建安全SQL查询 ==================
    QString DoctorId = DoctorComboBox->currentData().toString();
    // 动态拼接 SQL 过滤条件
    QString queryStr = QString("SELECT DATE(payment_date) AS day, SUM(amount) AS total "
                               "FROM financialRecords "
                               "WHERE payment_date BETWEEN :startDate AND :endDate "
                               "%1 GROUP BY day ORDER BY day"
                               ).arg(DoctorId != "-1" ? "AND doctor_id = :doctorId" : "");

    QSqlQuery query;
    query.prepare(queryStr);
    query.bindValue(":startDate", startDate.toString("yyyy-MM-dd"));
    query.bindValue(":endDate", endDate.toString("yyyy-MM-dd"));

    if (DoctorId != "-1") query.bindValue(":doctorId", DoctorId);

    if (!query.exec()) qCritical() << "[SQL错误]" << query.lastError().text();

    // ================== 3. 处理查询数据 ==================
    // 用 QMap 存储每个日期的总金额，同时计算最大金额以便后续设置 Y 轴
    QMap<QDate, qreal> dayData;
    qreal maxAmount = 0;
    while (query.next()) {
        QDate day = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
        if (!day.isValid()) continue;
        qreal amount = query.value(1).toDouble();
        dayData[day] = amount;
        if (amount > maxAmount) maxAmount = amount;
    }

    // ================== 4. 创建图表系列 ==================
    QLineSeries* series = new QLineSeries();
    series->setName("销售额");
    QPen pen(Qt::blue);
    series->setPen(pen);

    // 遍历从起始日期到结束日期的每一天，如果没有数据，就用 0 填补
    QDate currentDate = startDate;
    while (currentDate <= endDate) {
        qreal value = dayData.value(currentDate, 0.0);
        // QLineSeries 的 X 轴需要毫秒时间戳
        series->append(currentDate.startOfDay().toMSecsSinceEpoch(), value);
        currentDate = currentDate.addDays(1);
    }

    // ================== 5. 配置坐标轴 ==================
    QChart* chart = new QChart();
    chart->addSeries(series);

    // X 轴：时间轴
    QDateTimeAxis* axisX = new QDateTimeAxis();
    axisX->setFormat("yyyy-MM-dd");
    axisX->setTitleText("日期");
    axisX->setRange(startDate.startOfDay(), endDate.startOfDay());
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Y 轴：数值轴
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("金额 (元)");
    axisY->setLabelFormat("%.0f");

    // 如果不设置 axisY->setRange(0, maxAmount * 1.2)，折线可能会贴在图表的顶部边缘，很难看。
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // ================== 6. 应用图表 ==================
    // 清理旧图表，防止内存泄漏
    if (chartView->chart()) delete chartView->chart();
    chartView->setChart(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chart->legend()->setVisible(false);
}

void FinancialWidget::updatePieChart()
{
    // ================== 1. 获取筛选条件 ==================
    QString DoctorId = DoctorComboBox->currentData().toString();
    QDate startDate = startDateEdit->date();
    QDate endDate = endDateEdit->date();

    // ================== 2. 拼接 SQL 聚合查询 ==================
    // 根据支付类型(payment_type)分组，并求和(SUM)
    // 同样使用了 %3 占位符，如果选了具体医生，就追加 WHERE 过滤条件
    QString queryStr = QString(
                           "SELECT payment_type, SUM(amount) "
                           "FROM financialRecords "
                           "WHERE payment_date BETWEEN '%1' AND '%2' %3 "
                           "GROUP BY payment_type")
                           .arg(startDate.toString("yyyy-MM-dd"))
                           .arg(endDate.toString("yyyy-MM-dd"))
                           .arg(DoctorId != "-1" ? QString("AND doctor_id = '%1'").arg(DoctorId) : "");

    QSqlQuery query(queryStr);
    QPieSeries* series = new QPieSeries(); // 创建饼图的数据系列容器

    // ================== 3. 遍历查询结果，生成饼图数据 ==================
    while (query.next()) {
        QString type = query.value(0).toString();  // 支付类型（如：现金、转账）
        qreal value = query.value(1).toDouble();   // 该类型对应的总金额

        // 如果金额大于 0，才添加到饼图中
        if (value > 0) {
            // 设置图例显示的文本：如 "现金1000元"
            QString legendLabel = QString("%1%2元").arg(type).arg(value);
            QPieSlice* slice = new QPieSlice(legendLabel, value); // 创建饼图的一个扇形
            slice->setLabelVisible(false); // 不在扇形上显示标签（防止杂乱），标签统一在图例中显示
            series->append(slice);         // 将扇形添加到系列中
        }
    }

    // ================== 4. 配置 QChart 图表属性 ==================
    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("支付类型分布");

    // 设置图例（Legend）
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom); // 图例放到底部
    chart->legend()->setBackgroundVisible(true);
    chart->legend()->setBrush(QBrush(Qt::white));   // 图例背景为白色
    chart->legend()->setLabelColor(Qt::black);      // 图例文字颜色为黑色
    chart->legend()->setContentsMargins(10, 10, 10, 10); // 图例边距

    // 设置饼图大小：0.75 代表饼图占整个图表区域的 75%
    series->setPieSize(0.75);

    // ================== 5. 更新到 QChartView 视图 ==================
    // 关键点：QChartView 内部会接管传入的 chart 对象的所有权
    // 所以如果之前已经有 chart 对象，必须先 delete 掉，否则会发生内存泄漏
    if (pieChartView->chart()) {
        delete pieChartView->chart();
    }
    pieChartView->setChart(chart);

    // 强制重绘界面，确保饼图立即刷新显示
    pieChartView->repaint();
}

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




