#ifndef HONORWALLWIDGET_H
#define HONORWALLWIDGET_H
#include <QWidget>
#include <QLabel>
namespace Ui {
class HonorWallWidget;
}
class QPushButton;
class QGridLayout;
class QScrollArea;

constexpr int imgH = 500;
constexpr int imgW = 300;

#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>
#include <QMouseEvent>

class ClickableLabel : public QLabel {
    Q_OBJECT // 凡是带有 signals 或 slots 的类，必须加这个宏
public:
    // 构造函数：默认将父对象传进去，方便 Qt 的内存管理
    explicit ClickableLabel(QWidget* parent = nullptr) : QLabel(parent) {}

signals:
    // 自定义一个无参数的点击信号，供外部连接（比如双击、单击后去执行删除或放大逻辑）
    void clicked();

protected:
    // 重写鼠标按下事件
    void mousePressEvent(QMouseEvent* event) override {
        // 1. 不管鼠标左键、右键还是滚轮，只要按下去，就发送 clicked 信号
        emit clicked();

        // 2. 调用父类的同名函数，保证 QLabel 原有的默认行为（如拖拽、文本选中）不受影响
        QLabel::mousePressEvent(event);
    }
};

#endif // CLICKABLELABEL_H


class HonorWallWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HonorWallWidget(QWidget *parent = nullptr);
    ~HonorWallWidget();
private:
    void addImage();
    void modifyImage();
    void deleteImage();
    void loadImagesFromDatabase();
    void addImageToWall(const QString& imagePath);
    void addImageToUI(const QPixmap& pixmap);
    void onImageClicked();
    void reorderImages();
private:
    void setupUI();
    QPushButton *addButton;
    QPushButton *modifyButton;
    QPushButton *deleteButton;
    QScrollArea *scrollArea;
    QWidget *contentWidget;
    QGridLayout *gridLayout;
    Ui::HonorWallWidget *ui;
    ClickableLabel *selectedLabel = nullptr;
};

#endif // HONORWALLWIDGET_H
