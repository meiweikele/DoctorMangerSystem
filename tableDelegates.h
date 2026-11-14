#ifndef TABLEDELEGATES_H
#define TABLEDELEGATES_H

#include <QStyledItemDelegate>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QBuffer>
#include <QEvent>
#include <QFileDialog>
#include <QFile>
#include <QPainter>
#include <QMouseEvent>

// 1. 下拉框委托 (ComboBoxDelegate)
class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ComboBoxDelegate(QObject* parent = nullptr): QStyledItemDelegate(parent){}

    // 设置下拉菜单的选项（外部调用，传入如 {"男", "女"}）
    void setItems(const QStringList& items)
    {
        m_items = items;
    }

    // 当用户双击单元格进入编辑状态时，创建下拉框控件
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        QComboBox* editor = new QComboBox(parent);
        editor->addItems(m_items); // 将选项填充到下拉框
        return editor;
    }

    // 当编辑结束（如回车或点击别处）时，将下拉框选中的值写回数据模型
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        QComboBox* comboBox = static_cast<QComboBox*>(editor);
        QString value = comboBox->currentText();
        model->setData(index, value, Qt::EditRole); // 保存文本
    }
private:
    QStringList m_items; // 存储下拉菜单的选项
};


// 2. 日期选择委托 (DateEditDelegate)
class DateEditDelegate : public QStyledItemDelegate {
public:
    explicit DateEditDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    // 用户双击进入编辑状态时，创建一个日历控件
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override {
        Q_UNUSED(option);
        Q_UNUSED(index);
        QDateEdit* editor = new QDateEdit(parent);
        editor->setDisplayFormat("yyyy-MM-dd"); // 设置日期显示格式
        editor->setCalendarPopup(true);         // 点击下拉箭头时弹出日历
        return editor;
    }

    // 编辑结束，将选中的日期格式化后保存到模型
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
        QDateEdit* dateEdit = qobject_cast<QDateEdit*>(editor);
        if (dateEdit)
            model->setData(index, dateEdit->date().toString("yyyy-MM-dd"), Qt::EditRole);
    }
};



// 3. 图片处理委托 (ImageDelegate) - 最复杂的一个
class ImageDelegate : public QStyledItemDelegate {
public:
    explicit ImageDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    // 创建编辑器（其实这里用的是 QLabel 展示图片，作为编辑的载体）
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override {
        Q_UNUSED(option);
        Q_UNUSED(index);
        QLabel* editor = new QLabel(parent);
        return editor;
    }

    // 当编辑结束后，将 Label 里的图片转成二进制数据，存入模型
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
        QLabel* label = qobject_cast<QLabel*>(editor);
        if (label) {
            QByteArray imageData;
            // 注意：这里用的是 label->pixmap()，在 Qt6 中可能返回 QPixmap
            QPixmap pixmap = label->pixmap();
            if (!pixmap.isNull()) {
                QBuffer buffer(&imageData);
                buffer.open(QIODevice::WriteOnly);
                pixmap.save(&buffer, "PNG"); // 转成 PNG 二进制流
            }
            // 将二进制图片数据保存到 UserRole 角色中
            model->setData(index, imageData, Qt::UserRole);
        }
    }

    // 自定义绘制：当表格刷新时，如何把图片画在单元格里
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        // 从模型中提取二进制图片数据
        QByteArray imageData = index.data(Qt::UserRole).toByteArray();

        // 如果没有图片，就按默认方式绘制（画文字或者空白）
        if (imageData.isEmpty()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QPixmap pixmap;
        pixmap.loadFromData(imageData);
        if (pixmap.isNull()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        // 绘制图片：将其缩放到 100x100 大小，并绘制在单元格矩形内
        QRect rect = option.rect;
        QPixmap scaledPixmap = pixmap.scaled(100, 100, Qt::KeepAspectRatio);
        painter->drawPixmap(rect, scaledPixmap);
    }

    // 处理鼠标双击事件：双击单元格弹出文件选择框，更换图片
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                     const QModelIndex& index) override {
        // 如果是鼠标双击事件
        if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                // 弹出文件选择框
                QString imagePath = QFileDialog::getOpenFileName(
                    nullptr, "选择图片", "", "图片文件 (*.png *.jpg *.bmp)"
                    );

                // 如果用户选中了文件
                if (!imagePath.isEmpty()) {
                    QFile file(imagePath);
                    if (file.open(QIODevice::ReadOnly)) {
                        // 读取图片文件为二进制数据
                        QByteArray imageData = file.readAll();
                        file.close();
                        // 更新模型中的数据，触发界面重绘
                        model->setData(index, imageData, Qt::UserRole);
                    }
                }
                return true; // 告诉系统事件已被处理，不需要继续传递
            }
        }
        // 其他事件交给父类处理
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
};

#endif // TABLEDELEGATES_H