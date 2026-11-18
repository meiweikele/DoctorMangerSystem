#include "systemsettingwidget.h"
#include "ui_systemsettingwidget.h"
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QTextEdit>
#include <QGridLayout>
#include <QLabel>
#include <QFileDialog>
#include "settings.h"
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
SystemSettingWidget::SystemSettingWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SystemSettingWidget)
{
    setFixedSize(400,600);
    ui->setupUi(this);
    createUI();
    loadSettings();
}

SystemSettingWidget::~SystemSettingWidget()
{
    delete ui;
}

void SystemSettingWidget::createUI()
{
    dbPathEdit = new QLineEdit(this);
    browseBtn = new QPushButton("浏览...", this);
    oldPwdEdit = new QLineEdit(this);
    newPwdEdit = new QLineEdit(this);
    confirmPwdEdit = new QLineEdit(this);
    cacheCheckBox = new QCheckBox("记住登录信息", this);
    saveBtn = new QPushButton("保存", this);
    versionInfoEdit = new QTextEdit(this);

    oldPwdEdit->setEchoMode(QLineEdit::Password);
    newPwdEdit->setEchoMode(QLineEdit::Password);
    confirmPwdEdit->setEchoMode(QLineEdit::Password);

    versionInfoEdit->setPlainText("医生管理系统 1.0\n 开发环境：QT C++ 6.6，Qt Creator 20.0.2，Win11");
    versionInfoEdit->setReadOnly(true);

    mainLayout = new QGridLayout(this);
    mainLayout->addWidget(new QLabel("数据库路径:", this), 0, 0);
    mainLayout->addWidget(dbPathEdit, 0, 1);
    mainLayout->addWidget(browseBtn, 0, 2);
    mainLayout->addWidget(new QLabel("旧密码:", this), 1, 0);
    mainLayout->addWidget(oldPwdEdit, 1, 1, 1, 2);
    mainLayout->addWidget(new QLabel("新密码:", this), 2, 0);
    mainLayout->addWidget(newPwdEdit, 2, 1, 1, 2);
    mainLayout->addWidget(new QLabel("确认密码:", this), 3, 0);
    mainLayout->addWidget(confirmPwdEdit, 3, 1, 1, 2);
    mainLayout->addWidget(cacheCheckBox, 4, 0, 1, 3);
    mainLayout->addWidget(saveBtn, 5, 1, 1, 2);
    mainLayout->addWidget(versionInfoEdit, 6, 0, 1, 3);
    setLayout(mainLayout);
    connect(browseBtn, &QPushButton::clicked, this, &SystemSettingWidget::browseDatabasePath);
    connect(saveBtn, &QPushButton::clicked, this, &SystemSettingWidget::saveSettings);

}
// ==========================================================
// 1. 选择数据库存储位置 (browseDatabasePath)
// ==========================================================
void SystemSettingWidget::browseDatabasePath()
{
    // 弹出文件保存对话框，允许用户选择或创建一个新的 .db 文件路径
    QString path = QFileDialog::getSaveFileName(
        this,
        "选择数据库文件",
        "",
        "SQLite Databases (*.db *.sqlite)"
        );

    // 如果用户没有点取消（路径不为空），就把路径显示在输入框里
    if (!path.isEmpty()) dbPathEdit->setText(path);
}

// ==========================================================
// 2. 保存系统设置 (saveSettings)
// ==========================================================
void SystemSettingWidget::saveSettings()
{
    QString newDbPath = dbPathEdit->text();
    QString oldDbPath = Settings::instance().getDatabasePath(); // 先拿旧路径

    // 将新路径和缓存勾选状态保存到全局配置中
    Settings::instance().setDatabasePath(newDbPath);
    Settings::instance().setCacheEnabled(cacheCheckBox->isChecked());

    // 如果用户输入了新密码，则执行更新密码逻辑
    if (!newPwdEdit->text().isEmpty()) {
        updatePassword();
    }

    // 提示用户路径的修改需要重启软件才能生效
    if (newDbPath != oldDbPath) { // 和旧路径比对
        QMessageBox::information(this, "提示", "数据库路径修改将在重启后生效");
    }
}

// ==========================================================
// 3. 加载当前设置 (loadSettings)
// ==========================================================
void SystemSettingWidget::loadSettings()
{
    // 软件启动时，将配置中的路径和缓存状态回显到界面上
    dbPathEdit->setText(Settings::instance().getDatabasePath());
    cacheCheckBox->setChecked(Settings::instance().getCacheEnabled());
}

// ==========================================================
// 4. 修改密码 (updatePassword)
// ==========================================================
void SystemSettingWidget::updatePassword()
{
    // 先调用校验函数，如果旧密码不对或两次新密码不一致，直接退出
    if (!validatePasswordChange()) return;

    // 将新密码进行 SHA-256 哈希
    QString newHash = QString(QCryptographicHash::hash(
                                  newPwdEdit->text().toUtf8(),
                                  QCryptographicHash::Sha256
                                  ).toHex());

    // 更新数据库中当前用户的密码
    QSqlQuery query;
    query.prepare("UPDATE users SET password = ? WHERE username = ?");
    query.addBindValue(newHash);
    query.addBindValue(Settings::instance().getLastUser());

    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "密码更新失败: " + query.lastError().text());
        return;
    }
    QMessageBox::information(this, "提示", "密码更新成功");
}

// ==========================================================
// 5. 密码有效性验证 (validatePasswordChange)
// ==========================================================
bool SystemSettingWidget::validatePasswordChange()
{
    // 检查两次输入的新密码是否一致
    if (newPwdEdit->text() != confirmPwdEdit->text()) {
        QMessageBox::warning(this, "错误", "新密码与确认密码不一致");
        return false;
    }

    // 检查当前用户是否有效
    QString currentUser = Settings::instance().getLastUser();
    if (currentUser.isEmpty()) {
        QMessageBox::warning(this, "错误", "未找到当前用户");
        return false;
    }

    // 从数据库中查询该用户当前的真实密码哈希
    QSqlQuery query;
    query.prepare("SELECT password FROM users WHERE username = ?");
    query.addBindValue(currentUser);
    if (!query.exec() || !query.next()) {
        QMessageBox::critical(this, "错误", "数据库查询失败: " + query.lastError().text());
        return false;
    }
    QString storedHash = query.value(0).toString();

    // 将用户输入的旧密码进行哈希，与数据库中的哈希比对
    QString inputHash = QString(QCryptographicHash::hash(
                                    oldPwdEdit->text().toUtf8(),
                                    QCryptographicHash::Sha256
                                    ).toHex());

    if (storedHash != inputHash) {
        QMessageBox::warning(this, "错误", "旧密码不正确");
        return false;
    }
    return true; // 全部校验通过
}