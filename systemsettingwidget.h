#ifndef SYSTEMSETTINGWIDGET_H
#define SYSTEMSETTINGWIDGET_H

#include <QWidget>
class QLineEdit;
class QPushButton;
class QCheckBox;
class QTextEdit;
class QGridLayout;
namespace Ui {
class SystemSettingWidget;
}

class SystemSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SystemSettingWidget(QWidget *parent = nullptr);
    SystemSettingWidget(const SystemSettingWidget &) = delete;
    SystemSettingWidget(SystemSettingWidget &&) = delete;
    SystemSettingWidget &operator=(const SystemSettingWidget &) = delete;
    SystemSettingWidget &operator=(SystemSettingWidget &&) = delete;
    ~SystemSettingWidget();

private:
    void createUI();
    void browseDatabasePath();
    void saveSettings();
    void loadSettings();
    void updatePassword();
    bool validatePasswordChange();
private:
    QLineEdit* dbPathEdit;
    QPushButton* browseBtn;
    QLineEdit* oldPwdEdit;
    QLineEdit* newPwdEdit;
    QLineEdit* confirmPwdEdit;
    QCheckBox* cacheCheckBox;
    QPushButton* saveBtn;
    QTextEdit* versionInfoEdit;
    QGridLayout* mainLayout;

    Ui::SystemSettingWidget *ui;
};

#endif // SYSTEMSETTINGWIDGET_H
