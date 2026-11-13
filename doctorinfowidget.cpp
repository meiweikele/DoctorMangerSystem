#include "doctorinfowidget.h"
#include "ui_doctorinfowidget.h"
#include <QSqlQuery>
DoctorInfoWidget::DoctorInfoWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DoctorInfoWidget)
{
    ui->setupUi(this);
    refreshTable();
}

DoctorInfoWidget::~DoctorInfoWidget()
{
    delete ui;
}

void DoctorInfoWidget::refreshTable()
{
    // 1. 暂时阻断表格控件的信号，防止在插入数据的过程中频繁触发 itemChanged 等信号导致界面卡顿或逻辑错误
    ui->tableWidget->blockSignals(true);

    // 2. 清空当前表格的所有行，为重新加载数据做准备
    ui->tableWidget->setRowCount(0);

    // 3. 执行 SQL 查询，获取 DoctorInfo 表中的所有数据
    QSqlQuery query("SELECT * FROM DoctorInfo");

    // 4. 遍历查询结果集（每次循环处理一行记录）
    while (query.next())
    {
        // 获取当前表格的总行数（作为新数据即将插入的行索引）
        int row = ui->tableWidget->rowCount();

        // 在表格末尾插入一个新空行
        ui->tableWidget->insertRow(row);

        // 5. 遍历当前行的所有列，根据列索引填充数据
        for (int col = 0; col < ui->tableWidget->columnCount(); ++col)
        {
            // 创建一个新的表格项（单元格）
            QTableWidgetItem* item = new QTableWidgetItem();  //它完全依赖于 QTableWidget 这个控件的 setItem 方法来管理生命周期。只要你调用了 setItem，它的内存安全就由 Qt 帮你保证了。

            // 设置单元格内容居中对齐
            item->setTextAlignment(Qt::AlignCenter);

            // 6. 判断当前列是否是最后一列（通常约定最后一列为照片/图片列）
            if (col == ui->tableWidget->columnCount() - 1)
            {
                // --- 处理照片列 ---
                // 从数据库当前记录的当前列中读取图片数据（BLOB格式），转为 QByteArray
                QByteArray photoData = query.value(col).toByteArray();

                // 如果读取到的图片数据不为空
                if (!photoData.isEmpty())
                {
                    QPixmap photo;
                    // 从二进制数据中加载图片
                    photo.loadFromData(photoData);

                    // 将图片缩放到最大 100x100 尺寸（保持原图宽高比），并作为图标(DecorationRole)设置到单元格中
                    item->setData(Qt::DecorationRole, photo.scaled(100, 100, Qt::KeepAspectRatio));

                    // 将原始的二进制图片数据保存在单元格的 UserRole 属性中（方便后续需要导出或修改时使用）
                    item->setData(Qt::UserRole, photoData);
                }
            }
            else
            {
                // --- 处理普通文本列 ---
                // 读取数据库中的字符串数据，并设置到单元格文本中
                item->setText(query.value(col).toString());
            }

            // 7. 将配置好的 item 设置到表格的指定行和列中
            ui->tableWidget->setItem(row, col, item);
        }
    }

    // 8. 数据加载完毕，恢复表格的信号接收
    ui->tableWidget->blockSignals(false);
}