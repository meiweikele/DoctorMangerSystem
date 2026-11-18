#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
class QLineEdit;
class QPushButton;
namespace Ui {
class loginDialog;
}
const QByteArray encryptionKey = "your_encryption_key";// 加密和解密的密钥
class loginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit loginDialog(QWidget *parent = nullptr);

    ~loginDialog();

private:
    void checkAndCreateInitialUser();
    QString hashPassword(const QString& password);
    void on_loginButton_clicked();
    bool validateUser(const QString& username, const QString& password);
    void saveCredentials(const QString& username, const QString& password);
    QString encryptPassword(const QString& password);
    QString decryptPassword(const QString& encryptedPassword);
    bool loadCredentials(QString& username, QString& password);
private:
    Ui::loginDialog *ui;

    QLineEdit* usernameLineEdit;
    QLineEdit* passwordLineEdit;
    QPushButton* loginButton;
    QPushButton* cancelButton;

};

#endif // LOGINDIALOG_H
