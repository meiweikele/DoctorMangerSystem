#include "schedulewidget.h"
#include "ui_schedulewidget.h"
#include <QDate>
#include <QTableWidget>
#include <QComboBox>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSqlQuery>
#include <QMessageBox>
#include <QFormLayout>
#include <QTimeEdit>
#include <QSqlError>
#include <QTableWidgetItem>
//自定义周数计算:基于每年的1月1日作为起点，计算从该日期到给定日期的天数，并根据天数计算周数。
int customWeekNumber(const QDate& date) {
    // 1. 获取当前日期所在年份的 1 月 1 日
    QDate startOfYear(date.year(), 1, 1);
    // 2. 获取 1 月 1 日是星期几（周一返回1，周日返回7）
    int dayOfWeek = startOfYear.dayOfWeek();
    // 3. 计算当前日期距离 1 月 1 日过了多少天
    int days = startOfYear.daysTo(date);
    // 4. 核心算法：计算周数
    int week = (days + dayOfWeek - 1) / 7 + 1;
    return week;
}

ScheduleWidget::ScheduleWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ScheduleWidget)
{
    ui->setupUi(this);
    setupUI();
    int currentYear = QDate::currentDate().year();
    int currentWeek = customWeekNumber(QDate::currentDate());

    yearComboBox->setCurrentText(QString::number(currentYear));
    weekComboBox->setCurrentText(QString("第 %1 周").arg(currentWeek));

    loadSchedule();

}

ScheduleWidget::~ScheduleWidget()
{
    delete ui;
}

void ScheduleWidget::setupUI()
{
    // 1. 主布局初始化
    // 创建主垂直布局，并将父对象设为当前窗口（this）
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 创建用于放置“年份、周数、日期范围”的水平布局
    QHBoxLayout* dateLayout = new QHBoxLayout();

    //  2. 顶部日期选择区
    yearComboBox = new QComboBox(this); // 年份下拉框
    weekComboBox = new QComboBox(this); // 周数下拉框

    // 填充年份下拉框：从 2020 年填充到 当前年份 + 5 年
    int currentYear = QDate::currentDate().year();
    for (int year = 2020; year <= currentYear + 5; ++year)
        yearComboBox->addItem(QString::number(year), year); // 显示文本，实际绑定的数据

    // 填充周数下拉框：1 到 52 周
    for (int week = 1; week <= 52; ++week)
        weekComboBox->addItem(QString("第 %1 周").arg(week), week);

    // 用于显示所选周的具体日期范围（例如：2023-10-23 ~ 2023-10-29）
    dateRangeLabel = new QLabel(this);

    // 创建周导航按钮
    QPushButton* prevWeekBtn = new QPushButton("上一周", this);
    QPushButton* nextWeekBtn = new QPushButton("下一周", this);
    prevWeekBtn->setFixedWidth(200);
    nextWeekBtn->setFixedWidth(200);

    // 将组件添加到顶部水平布局中
    dateLayout->addWidget(new QLabel("年份：", this));
    dateLayout->addWidget(yearComboBox);
    dateLayout->addWidget(new QLabel("周数：", this));
    dateLayout->addWidget(weekComboBox);
    dateLayout->addWidget(dateRangeLabel);
    dateLayout->addStretch(); // 添加弹簧，把组件往左边挤

    //  3. 课程表格初始化
    tableWidget = new QTableWidget(this);
    tableWidget->setAlternatingRowColors(true); // 开启交替行变色（斑马线效果）
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection); // 设置只能单选行
    setupTable(); // 调用另一个函数，配置表格的行列、表头等结构

    // 4. 底部操作按钮初始化
    addButton = new QPushButton("添加课程", this);
    deleteButton = new QPushButton("删除课程", this);
    addButton->setFixedWidth(200);
    deleteButton->setFixedWidth(200);

    //  5. 信号与槽连接
    // 当用户切换年份或周数时，触发 loadSchedule 重新加载课程数据
    // QOverload<int>::of 是为了解决 currentIndexChanged 信号有多个重载版本的问题
    connect(yearComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScheduleWidget::loadSchedule);
    connect(weekComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScheduleWidget::loadSchedule);

    // 点击添加按钮时，触发添加课程逻辑
    connect(addButton, &QPushButton::clicked, this, &ScheduleWidget::addCourse);

    connect(deleteButton, &QPushButton::clicked, this, &ScheduleWidget::deleteCourse);
    connect(prevWeekBtn, &QPushButton::clicked, this, &ScheduleWidget::showPreviousWeek);
    connect(nextWeekBtn, &QPushButton::clicked, this, &ScheduleWidget::showNextWeek);
    connect(tableWidget, &QTableWidget::itemChanged, this, &ScheduleWidget::handleItemChanged);

    //  6. 底部按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(); // 左侧弹簧
    buttonLayout->addWidget(prevWeekBtn);
    buttonLayout->addWidget(nextWeekBtn);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch(); // 右侧弹簧，让按钮整体居中

    //  7. 组装主布局
    mainLayout->addLayout(dateLayout);        // 顶部：日期选择
    mainLayout->addWidget(tableWidget);       // 中部：课程表格
    mainLayout->addLayout(buttonLayout);      // 底部：操作按钮

    setLayout(mainLayout); // 将主布局应用到当前窗口（其实在第一步传了 this 可以省略这一步）
}

void ScheduleWidget::loadSchedule()
{
    // 1. 界面防抖动预处理
    // 暂时阻断表格的信号（如 itemChanged），防止后续插入数据时触发不必要的业务逻辑（死循环或卡顿）
    tableWidget->blockSignals(true);
    // 清空表格里原有的所有文字内容（保留表头和行列结构）
    tableWidget->clearContents();

    //  2. 获取并计算当前选择的周日期范围
    // 从下拉框的隐藏数据 (UserRole) 中取出真实的整型年份和周数
    int year = yearComboBox->currentData().toInt();
    int week = weekComboBox->currentData().toInt();

    // 调用辅助函数，算出这一周的周一和周日分别是几号
    QPair<QDate, QDate> weekRange = getWeekRange(year, week);
    QDate startDate = weekRange.first;
    QDate endDate = weekRange.second;

    // 在界面的 QLabel 上更新显示（例如："2023-10-23到2023-10-29"）
    dateRangeLabel->setText(startDate.toString("yyyy-MM-dd") + "到" + endDate.toString("yyyy-MM-dd"));

    // 3. 准备二维数据容器
    // 创建一个 7行(一周7天) × N列(课程节数) 的二维数组，初始值为空字符串 ""
    // 这相当于一个临时的内存网格，等数据填满后再统一渲染到界面上，效率更高
    QVector<QVector<QString>> courses(7, QVector<QString>(times.count(), ""));

    //  4. 从数据库查询本周课程
    QSqlQuery query;
    query.prepare("SELECT date, time, course_name FROM schedule WHERE date BETWEEN ? AND ?");
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    if (query.exec()) {
        // 遍历查询结果集
        while (query.next()) {
            QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
            QString time = query.value(1).toString();

            // 计算映射关系：
            // dayIndex：今天距离这周一过了几天？（0代表周一，6代表周日）
            int dayIndex = startDate.daysTo(date);
            // timeIndex：这节课在 times 数组（如 {"第1节", "第2节"...}）里的索引位置
            int timeIndex = times.indexOf(time);

            // 防止脏数据导致数组越界崩溃
            if (dayIndex >= 0 && dayIndex < 7 && timeIndex != -1) {
                // 将课程名填入内存网格的对应位置
                courses[dayIndex][timeIndex] = query.value(2).toString();
            }
        }
    }

    // 5. 将数据渲染到 UI 表格
    // 遍历内存网格
    for (int day = 0; day < 7; ++day) {
        for (int time = 0; time < times.count(); ++time) {
            // 为每个格子创建一个表格项
            QTableWidgetItem* item = new QTableWidgetItem(courses[day][time]);
            item->setTextAlignment(Qt::AlignCenter); // 文字居中
            // 将 item 塞入表格的指定行列
            tableWidget->setItem(day, time, item);
        }
    }

    //  6. 恢复界面信号
    tableWidget->blockSignals(false);
}



// 1. 初始化表格结构 (setupTable)
void ScheduleWidget::setupTable() {
    // 定义行表头（星期几）和列表头（上课时间段）
    QStringList days = { "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日" };
    times = { "上午1", "上午2", "下午1", "下午2", "晚上1", "晚上2" };

    // 根据数组大小设置表格的行数和列数
    tableWidget->setRowCount(days.count());
    tableWidget->setColumnCount(times.count());

    // 获取当前下拉框选中的年份和周数，计算出这一周的日期范围
    int year = yearComboBox->currentData().toInt();
    int week = weekComboBox->currentData().toInt();
    QPair<QDate, QDate> weekRange = getWeekRange(year, week);
    QDate startDate = weekRange.first; // 这一周的星期一

    // 动态生成左侧行表头（如："星期一\n10/23"）
    QStringList verticalHeaders;
    for (int i = 0; i < days.count(); ++i) {
        QDate currentDate = startDate.addDays(i); // 计算出对应星期的具体日期
        // 拼接显示：换行显示星期几和具体日期(月/日)
        verticalHeaders.append(QString("%1\n%2").arg(days[i]).arg(currentDate.toString("MM/dd")));
    }

    // 应用表头
    tableWidget->setVerticalHeaderLabels(verticalHeaders);
    tableWidget->setHorizontalHeaderLabels(times);

    // 设置表头自适应拉伸，铺满整个表格空间
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 设置编辑触发器：只有双击单元格 或 按键盘编辑键（如F2） 才能进入编辑模式
    tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
}

// 2. 添加课程 (addCourse)
void ScheduleWidget::addCourse()
{
    // 获取用户当前选中的行（星期几）和列（第几节课）
    int dayIndex = tableWidget->currentRow();
    int timeIndex = tableWidget->currentColumn();

    // 验证是否选中了有效单元格
    if (dayIndex == -1 || timeIndex == -1) {
        QMessageBox::warning(this, "错误", "请先选择一个时间段！");
        return;
    }

    // 检查该单元格是否已经有课程（防止覆盖）
    if (!tableWidget->item(dayIndex, timeIndex)->text().isEmpty()) {
        QMessageBox::warning(this, "错误", "该时间段已被占用！");
        return;
    }

    // 构建弹窗对话框
    QDialog dialog(this);
    dialog.setWindowTitle("添加课程");
    QFormLayout layout(&dialog); // 表单布局

    // 医生姓名下拉框：从数据库读取所有医生姓名
    QComboBox nameCombo;
    QSqlQuery nameQuery("SELECT name FROM DoctorInfo");
    while (nameQuery.next()) nameCombo.addItem(nameQuery.value(0).toString());

    // 时间预设映射表：根据列索引，自动填充一个默认时间，减少用户输入工作量
    QMap<int, QTime> timePresets = {
        {0, QTime(9, 0)}, {1, QTime(11, 0)}, {2, QTime(14, 0)},
        {3, QTime(16, 0)}, {4, QTime(19, 0)}, {5, QTime(21, 0)}
    };

    // 时间选择控件
    QTimeEdit timeEdit;
    timeEdit.setDisplayFormat("HH:mm");
    timeEdit.setTime(timePresets.value(timeIndex)); // 根据当前点击的列设置默认时间

    // 将控件加入表单
    layout.addRow("医生姓名:", &nameCombo);
    layout.addRow("课程时间:", &timeEdit);

    // 确认/取消按钮组
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons.button(QDialogButtonBox::Ok)->setText("确定");
    buttons.button(QDialogButtonBox::Cancel)->setText("取消");
    layout.addRow(&buttons);

    // 绑定按钮信号与对话框槽函数
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 如果用户点击了取消，直接退出函数
    if (dialog.exec() != QDialog::Accepted) return;
    // 到这里说明用户点击了确定

    //  数据保存
    // 将姓名和时间组合成一个字符串，例如："张三,14:00"
    QString courseName = QString("%1,%2").arg(nameCombo.currentText())
                             .arg(timeEdit.time().toString("HH:mm"));

    // 再次计算日期，确定要插入到哪一天的数据库记录里
    int year = yearComboBox->currentData().toInt();
    int week = weekComboBox->currentData().toInt();
    QPair<QDate, QDate> weekRange = getWeekRange(year, week);
    QDate currentDate = weekRange.first.addDays(dayIndex); // 计算出用户点击的那一列的具体日期
    QString timeSlot = times[timeIndex]; // 原始时间段标识（如"上午1"）

    // 写入数据库
    QSqlQuery query;
    // 注意：这里存入的 time 是 "上午1" 这种标识，而 courseName 里是 "姓名,HH:mm"
    query.prepare("INSERT INTO schedule (date, time, course_name) VALUES (?, ?, ?)");
    query.addBindValue(currentDate.toString("yyyy-MM-dd"));
    query.addBindValue(timeSlot);
    query.addBindValue(courseName);

    if (!query.exec())
        QMessageBox::critical(this, "错误", "添加失败：" + query.lastError().text());
    else
        loadSchedule(); // 插入成功后，重新加载数据，刷新界面显示
}


// 1. 删除选中课程 (deleteCourse)
void ScheduleWidget::deleteCourse()
{
    //  1. 弹出确认对话框
    QMessageBox confirmBox(this);
    confirmBox.setWindowTitle("确认删除");
    confirmBox.setText("确定要删除该记录吗？");

    // 自定义按钮文字（防止英文 Yes/No 带来不友好的体验）
    QPushButton* yesButton = confirmBox.addButton("确定", QMessageBox::YesRole);
    QPushButton* noButton = confirmBox.addButton("取消", QMessageBox::NoRole);

    // 默认将焦点放在“取消”上，防止用户误按回车直接删除
    confirmBox.setDefaultButton(noButton);

    // 模态显示对话框，阻塞等待用户操作
    confirmBox.exec();

    //  2. 用户确认后的删除逻辑
    if (confirmBox.clickedButton() == yesButton) {
        // 获取当前选中的行（星期几）和列（第几节课）
        int dayIndex = tableWidget->currentRow();
        int timeIndex = tableWidget->currentColumn();

        // 检查是否选中了有效单元格
        if (dayIndex == -1 || timeIndex == -1) {
            QMessageBox::warning(this, "错误", "请先选择一个时间段！");
            return;
        }

        // 检查该单元格是否有数据（是否为空）
        QTableWidgetItem* item = tableWidget->item(dayIndex, timeIndex);
        if (!item || item->text().isEmpty()) {
            QMessageBox::warning(this, "错误", "该时间段没有课程！");
            return;
        }

        // 3. 根据选中的位置计算日期和时间并删除
        int year = yearComboBox->currentData().toInt();
        int week = weekComboBox->currentData().toInt();
        QPair<QDate, QDate> weekRange = getWeekRange(year, week);
        QDate currentDate = weekRange.first.addDays(dayIndex); // 换算成具体日期
        QString time = times[timeIndex]; // 例如 "上午1"

        // 执行 SQL 删除
        QSqlQuery query;
        query.prepare("DELETE FROM schedule WHERE date = ? AND time = ?");
        query.addBindValue(currentDate.toString("yyyy-MM-dd"));
        query.addBindValue(time);

        if (!query.exec()) {
            QMessageBox::critical(this, "错误", "删除失败：" + query.lastError().text());
        }
        else {
            loadSchedule(); // 刷新界面，让表格恢复空白
        }
    }
}


// 2. 直接编辑单元格内容时触发 (handleItemChanged)
void ScheduleWidget::handleItemChanged(QTableWidgetItem *item)
{
    // 获取被修改单元格的行和列
    int day = item->row();
    int timeSlot = item->column();
    QString newCourse = item->text().trimmed(); // 获取修改后的文本

    // 计算对应的实际日期
    int year = yearComboBox->currentData().toInt();
    int week = weekComboBox->currentData().toInt();
    QPair<QDate, QDate> weekRange = getWeekRange(year, week);
    QDate date = weekRange.first.addDays(day);
    QString time = times[timeSlot];

    QSqlQuery query;

    //  判断是删除还是新增/修改
    if (newCourse.isEmpty()) {
        // 【场景A】：用户把单元格里的字删光了，说明他想删除这门课
        query.prepare("DELETE FROM schedule WHERE date = ? AND time = ?");
        query.addBindValue(date.toString("yyyy-MM-dd"));
        query.addBindValue(time);
    }
    else {
        // 【场景B】：用户修改了文字，或者原本为空现在填了字
        // 使用 SQLite 的 INSERT OR REPLACE 语法（即主键冲突时执行 Update，否则执行 Insert）
        // 这要求你的表 PRIMARY KEY (date, time) 是存在的
        query.prepare("INSERT OR REPLACE INTO schedule (date, time, course_name) VALUES (?, ?, ?)");
        query.addBindValue(date.toString("yyyy-MM-dd"));
        query.addBindValue(time);
        query.addBindValue(newCourse);
    }

    //执行并容错
    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "操作失败：" + query.lastError().text());
        loadSchedule(); // 如果数据库操作失败，强制刷新表格，把界面恢复成数据库里的真实状态
    }
}

// ==========================================================
// 1. 切换到上一周 (showPreviousWeek)
// ==========================================================
void ScheduleWidget::showPreviousWeek()
{
    // 获取当前下拉框选中的索引（注意：index 是索引，从 0 开始）
    int currentWeek = weekComboBox->currentIndex();
    int currentYear = yearComboBox->currentIndex();

    // 如果当前不是第 1 周（索引大于 0）
    if (currentWeek > 0) {
        // 简单粗暴，直接切到上一周
        weekComboBox->setCurrentIndex(currentWeek - 1);
    }
    else {
        // 如果当前已经是第 1 周，说明要跨年了，跳到上一年的最后一周
        // 前提是年份下拉框不是第一项（还有更早的年份）
        if (yearComboBox->currentIndex() > 0) {
            yearComboBox->setCurrentIndex(currentYear - 1); // 年份减 1
            // 跳转到上一年最后一周（索引 51 代表第 52 周，因为 addItem 是从 1 到 52）
            weekComboBox->setCurrentIndex(51);
        }
    }
}

// ==========================================================
// 2. 切换到下一周 (showNextWeek)
// ==========================================================
void ScheduleWidget::showNextWeek()
{
    int currentWeek = weekComboBox->currentIndex();
    int currentYear = yearComboBox->currentIndex();

    // 如果当前不是最后一周（索引小于 51，即不是第 52 周）
    if (currentWeek < 51) {
        // 直接切到下一周
        weekComboBox->setCurrentIndex(currentWeek + 1);
    }
    else {
        // 如果当前是最后一周，说明要跨年了，跳到下一年的第 1 周
        if (yearComboBox->currentIndex() < yearComboBox->count()-1) {
            yearComboBox->setCurrentIndex(currentYear + 1); // 年份加 1
            weekComboBox->setCurrentIndex(0); // 切到第 1 周（索引 0）
        }
    }
}

// 3. 计算周日期范围 (getWeekRange)
QPair<QDate, QDate> ScheduleWidget::getWeekRange(int year, int week) {
    // 获取该年的 1月1日
    QDate startDate(year, 1, 1);

    // 如果 1月1日 不是周一，就往前推到最近的周一
    // 例如：1月1日是周三，dayOfWeek为3，减去(3-1)=2天，得到上一年的12月30日（周一）
    int daysToSubtract = startDate.dayOfWeek() - Qt::Monday;
    if (daysToSubtract > 0) startDate = startDate.addDays(-daysToSubtract);

    // 根据周数向后推算，得到这周的周一和周日
    QDate weekStart = startDate.addDays((week - 1) * 7);
    QDate weekEnd = weekStart.addDays(6);

    // 返回包含开始和结束日期的键值对
    return qMakePair(weekStart, weekEnd);
}
