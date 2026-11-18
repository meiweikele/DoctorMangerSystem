#include "logindialog.h"
#include "ui_logindialog.h"
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QPushButton>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QMessageBox>
#include "settings.h"
loginDialog::loginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::loginDialog)
{
    ui->setupUi(this);
    checkAndCreateInitialUser(); // 检查数据库是否为空，若为空则插入初始用户
    setWindowTitle("医生管理系统");    // 设置窗口标题
    setWindowIcon(QIcon(":/ico/le.png"));
    setFixedSize(260, 180);
    // 创建控件
    QLabel* usernameLabel = new QLabel("用户名:", this);
    QLabel* passwordLabel = new QLabel("密   码:", this);
    usernameLineEdit = new QLineEdit(this);
    passwordLineEdit = new QLineEdit(this);
    passwordLineEdit->setEchoMode(QLineEdit::Password); // 密码输入模式
    loginButton = new QPushButton("登录", this);
    cancelButton = new QPushButton("取消", this);
    // 布局
    QGridLayout* mainLayout = new QGridLayout(this);
    mainLayout->addWidget(usernameLabel, 0, 0);
    mainLayout->addWidget(usernameLineEdit, 0, 1);
    mainLayout->addWidget(passwordLabel, 1, 0);
    mainLayout->addWidget(passwordLineEdit, 1, 1);
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(loginButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout, 2, 0, 1, 2); // 跨两列
    setLayout(mainLayout);
    // 连接信号和槽
    connect(loginButton, &QPushButton::clicked, this, &loginDialog::on_loginButton_clicked);
    connect(cancelButton, &QPushButton::clicked, this, &loginDialog::reject);
    // 尝试加载缓存的登录信息
    QString cachedUsername, cachedPassword;
    if (loadCredentials(cachedUsername, cachedPassword)) {
        usernameLineEdit->setText(cachedUsername);
        passwordLineEdit->setText(cachedPassword);
    }

}

loginDialog::~loginDialog()
{
    delete ui;
}
//创建用户名和密码输入框、登录和取消按钮，并进行合理布局。
void loginDialog::checkAndCreateInitialUser()
{
    // ================== 1. 定义初始账号和密码 ==================
    const QString initialUsername = "admin";       // 初始用户名
    const QString initialPassword = "admin123";    // 初始密码（明文）

    // ================== 2. 检查 users 表是否为空 ==================
    QSqlQuery query;
    query.exec("SELECT COUNT(*) FROM users"); // 查询 users 表的总记录数

    // 如果查询成功，并且 count 为 0（说明表里一个人都没有）
    if (query.next() && query.value(0).toInt() == 0) {

        // ================== 3. 加密初始密码 ==================
        // 调用之前写好的哈希函数，把 "admin123" 变成一堆乱码
        QString hashedInitialPassword = hashPassword(initialPassword);

        // ================== 4. 插入初始用户 ==================
        query.prepare("INSERT INTO users (username, password) VALUES (:username, :password)");
        query.bindValue(":username", initialUsername);
        query.bindValue(":password", hashedInitialPassword);

        if (!query.exec()) {
            // 如果插入失败，打印错误信息
            qDebug() << "插入初始用户失败:" << query.lastError().text();
        }
    }
}

QString loginDialog::hashPassword(const QString& password)
{
    // ================== 1. 字符串转字节数组 ==================
    // 密码在 C++ 的 QString 中是 Unicode 编码的（通常是 UTF-16），
    // 而加密算法只认原始的字节流，所以必须统一转成 UTF-8 格式的字节数组。
    QByteArray passwordBytes = password.toUtf8();

    // ================== 2. 执行 SHA-256 哈希计算 ==================
    // 调用 Qt 自带的加密类 QCryptographicHash
    // Sha256 是一种单向加密算法（不可逆），不管多长的密码，算出来都是一段 64 位长度的 16 进制字符串。
    QByteArray hashBytes = QCryptographicHash::hash(passwordBytes, QCryptographicHash::Sha256);

    // ================== 3. 转换为十六进制字符串 ==================
    // 刚刚算出来的是二进制乱码，转成 toHex()（十六进制字符）后，
    // 就可以像普通字符串一样直接存入数据库进行比对了。
    return QString(hashBytes.toHex());
}

// ==========================================================
// 1. 登录按钮点击槽函数 (on_loginButton_clicked):实现用户登录验证、登录信息保存功能
// ==========================================================
void loginDialog::on_loginButton_clicked()
{
    // 获取输入框中的用户名和明文密码
    QString username = usernameLineEdit->text();
    QString password = passwordLineEdit->text();

    // 调用验证函数去数据库比对
    if (validateUser(username, password)) {
        // 如果验证通过：
        saveCredentials(username, password); // 第一步：记住密码（保存到本地配置文件）
        Settings::instance().setLastUser(username); // 第二步：记录最后一次登录的用户名
        accept(); // 第三步：关闭登录对话框（返回 QDialog::Accepted，主窗口据此决定是否启动）
    }
    else {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误。");
    }
}

// ==========================================================
// 2. 验证用户函数 (validateUser)
// ==========================================================
bool loginDialog::validateUser(const QString& username, const QString& password) {
    // 将用户输入的明文密码进行 SHA-256 哈希，以便和数据库中的密文比对
    QString hashedPassword = hashPassword(password);

    QSqlQuery query;
    // 核心安全机制：用用户名和哈希后的密码一起去查数据库
    query.prepare("SELECT * FROM users WHERE username = :username AND password = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", hashedPassword);

    if (!query.exec()) {
        qDebug() << "查询错误:" << query.lastError().text();
        return false;
    }

    // query.next() 如果返回 true，说明查到了记录（登录成功）；返回 false 说明查无此人（登录失败）
    return query.next();
}

// ==========================================================
// 3. 保存登录凭据函数 (saveCredentials)
// ==========================================================
void loginDialog::saveCredentials(const QString& username, const QString& password) {
    // 使用 QSettings 把用户名存到本地配置文件（Windows 下存在注册表）
    Settings::instance().getQSettings().setValue("username", username);

    // ⚠️ 安全警告：虽然你在这里对密码进行了二次加密（encryptPassword），
    // 但如果这个加密算法是硬编码在程序里的对称加密（如 Base64 或简单的异或），
    // 黑客逆向你的程序后，就可以轻易解开这个密码。
    QString encryptedPassword = encryptPassword(password);
    Settings::instance().getQSettings().setValue("password", encryptedPassword);
}

// ==========================================================
// 1. 从本地读取并解密凭据 (loadCredentials): 加载缓存的用户名和加密后的密码
// ==========================================================
bool loginDialog::loadCredentials(QString& username, QString& password) {
    // 从 QSettings 中读取上一次登录的用户名
    username = Settings::instance().getQSettings().value("username").toString();
    // 读取加密后的一串乱码（Base64 字符串）
    QString encryptedPassword = Settings::instance().getQSettings().value("password").toString();

    // 只有当用户名和加密密码都存在时，才进行解密
    if (!username.isEmpty() && !encryptedPassword.isEmpty()) {
        // 调用解密函数，把乱码还原成明文密码
        password = decryptPassword(encryptedPassword);
        return true; // 告诉调用者：读取并解密成功
    }
    return false; // 说明是第一次登录，或者用户没勾选记住密码
}

// ==========================================================
// 2. 异或加密函数 (encryptPassword)
// ==========================================================
QString loginDialog::encryptPassword(const QString& password) {
    // 将明文密码转成 UTF-8 字节流
    QByteArray passwordBytes = password.toUtf8();
    QByteArray encryptedBytes;

    // 核心算法：逐字节与密钥(encryptionKey)进行异或(XOR)运算
    // i % encryptionKey.size() 确保密钥可以循环使用
    for (int i = 0; i < passwordBytes.size(); ++i) {
        encryptedBytes.append(passwordBytes[i] ^ encryptionKey[i % encryptionKey.size()]);
    }

    // 异或运算后可能产生不可打印的二进制字符，所以转成 Base64 字符串方便存入配置文件
    return encryptedBytes.toBase64();
}

// ==========================================================
// 3. 异或解密函数 (decryptPassword)
// ==========================================================
QString loginDialog::decryptPassword(const QString& encryptedPassword) {
    // 先把 Base64 字符串还原成原始的二进制字节流
    QByteArray encryptedBytes = QByteArray::fromBase64(encryptedPassword.toUtf8());
    QByteArray decryptedBytes;

    // 再次与同一个密钥进行异或运算。
    // 因为 XOR 的特性：(A ^ B) ^ B = A，所以能完美还原出原来的明文。
    for (int i = 0; i < encryptedBytes.size(); ++i) {
        decryptedBytes.append(encryptedBytes[i] ^ encryptionKey[i % encryptionKey.size()]);
    }

    // 将字节流转回 UTF-8 字符串返回
    return QString::fromUtf8(decryptedBytes);
}


