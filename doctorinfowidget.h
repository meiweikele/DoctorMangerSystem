#ifndef DOCTORINFOWIDGET_H
#define DOCTORINFOWIDGET_H

#include <QWidget>
#include <QSqlQuery>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialog>
#include <QByteArray>
#include <QTableWidgetItem>
class QGroupBox;   //定义了指针，因此可以不需要用头文件
namespace Ui {
class DoctorInfoWidget;
}

class DoctorInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DoctorInfoWidget(QWidget *parent = nullptr);
    ~DoctorInfoWidget();
private slots:
    void on_btnAdd_clicked();
    void on_btnDeleteItem_clicked();

    void on_btnDeleteLine_clicked();
    void handleItemChanged(QTableWidgetItem* item);
private:
    void refreshTable();
    QGroupBox* createFormGroup();
    QGroupBox* createPhotoGroup();
    void handleDialogAccepted(QGroupBox*formGroup, QGroupBox*photoGroup);

    QByteArray photoData;
private:
    Ui::DoctorInfoWidget *ui;
};

#endif // DOCTORINFOWIDGET_H
