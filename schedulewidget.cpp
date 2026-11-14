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

    //loadSchedule();

}

ScheduleWidget::~ScheduleWidget()
{
    delete ui;
}

void ScheduleWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QHBoxLayout* dateLayout = new QHBoxLayout();
    yearComboBox = new QComboBox(this);
    weekComboBox = new QComboBox(this);

    int currentYear = QDate::currentDate().year();
    for (int year = 2020; year <= currentYear + 5; ++year)
        yearComboBox->addItem(QString::number(year), year);

    for (int week = 1; week <= 52; ++week)
        weekComboBox->addItem(QString("第 %1 周").arg(week), week);

    dateRangeLabel = new QLabel(this);
    // 添加周导航按钮
    QPushButton* prevWeekBtn = new QPushButton("上一周", this);
    QPushButton* nextWeekBtn = new QPushButton("下一周", this);
    prevWeekBtn->setFixedWidth(200);
    nextWeekBtn->setFixedWidth(200);

    dateLayout->addWidget(new QLabel("年份：", this));
    dateLayout->addWidget(yearComboBox);
    dateLayout->addWidget(new QLabel("周数：", this));
    dateLayout->addWidget(weekComboBox);
    dateLayout->addWidget(dateRangeLabel);
    dateLayout->addStretch();

    tableWidget = new QTableWidget(this);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    setupTable();

    addButton = new QPushButton("添加课程", this);
    deleteButton = new QPushButton("删除课程", this);
    addButton->setFixedWidth(200);
    deleteButton->setFixedWidth(200);

    // connect(yearComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
    //         this, &ScheduleWidget::loadSchedule);
    // connect(weekComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
    //         this, &ScheduleWidget::loadSchedule);
    // connect(addButton, &QPushButton::clicked, this, &ScheduleWidget::addCourse);
    // connect(deleteButton, &QPushButton::clicked, this, &ScheduleWidget::deleteCourse);
    // connect(prevWeekBtn, &QPushButton::clicked, this, &ScheduleWidget::showPreviousWeek);
    // connect(nextWeekBtn, &QPushButton::clicked, this, &ScheduleWidget::showNextWeek);
    // connect(tableWidget, &QTableWidget::itemChanged, this, &ScheduleWidget::handleItemChanged);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(prevWeekBtn);
    buttonLayout->addWidget(nextWeekBtn);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(dateLayout);
    mainLayout->addWidget(tableWidget);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}


void ScheduleWidget::setupTable() {
    QStringList days = { "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日" };
    times = { "上午1", "上午2", "下午1", "下午2", "晚上1", "晚上2" };
    tableWidget->setRowCount(days.count());
    tableWidget->setColumnCount(times.count());
    int year = yearComboBox->currentData().toInt();
    int week = weekComboBox->currentData().toInt();
    QPair<QDate, QDate> weekRange = getWeekRange(year, week);
    QDate startDate = weekRange.first;
    QStringList verticalHeaders;
    for (int i = 0; i < days.count(); ++i) {
        QDate currentDate = startDate.addDays(i);
        verticalHeaders.append(QString("%1\n%2").arg(days[i]).arg(currentDate.toString("MM/dd")));
    }
    tableWidget->setVerticalHeaderLabels(verticalHeaders);
    tableWidget->setHorizontalHeaderLabels(times);
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
}

QPair<QDate, QDate> ScheduleWidget::getWeekRange(int year, int week) {
    QDate startDate(year, 1, 1);
    int daysToSubtract = startDate.dayOfWeek() - Qt::Monday;
    if (daysToSubtract > 0) startDate = startDate.addDays(-daysToSubtract);
    QDate weekStart = startDate.addDays((week - 1) * 7);
    QDate weekEnd = weekStart.addDays(6);
    return qMakePair(weekStart, weekEnd);
}

