#ifndef DOCTORINFOWIDGET_H
#define DOCTORINFOWIDGET_H

#include <QWidget>

namespace Ui {
class DoctorInfoWidget;
}

class DoctorInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DoctorInfoWidget(QWidget *parent = nullptr);
    ~DoctorInfoWidget();
private:
    void refreshTable();

private:
    Ui::DoctorInfoWidget *ui;
};

#endif // DOCTORINFOWIDGET_H
