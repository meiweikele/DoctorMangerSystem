#include "honorwallwidget.h"
#include "ui_honorwallwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include <QGridLayout>
#include <QSqlQuery>
HonorWallWidget::HonorWallWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HonorWallWidget)
{
    ui->setupUi(this);
    setupUI();
    loadImagesFromDatabase();
}

HonorWallWidget::~HonorWallWidget()
{
    delete ui;
}

void HonorWallWidget::addImage()
{

}

void HonorWallWidget::modifyImage()
{

}

void HonorWallWidget::deleteImage()
{

}

void HonorWallWidget::setupUI()
{
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    // 添加按钮
    addButton = new QPushButton("添加图片", this);
    connect(addButton, &QPushButton::clicked, this, &HonorWallWidget::addImage);
    buttonLayout->addWidget(addButton);
    // 修改按钮
    modifyButton = new QPushButton("修改图片", this);
    connect(modifyButton, &QPushButton::clicked, this, &HonorWallWidget::modifyImage);
    buttonLayout->addWidget(modifyButton);
    // 删除按钮
    deleteButton = new QPushButton("删除图片", this);
    connect(deleteButton, &QPushButton::clicked, this, &HonorWallWidget::deleteImage);
    buttonLayout->addWidget(deleteButton);
    mainLayout->addLayout(buttonLayout);
    // 滚动区域
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true); // 允许内容区域调整大小
    // 内容区域
    contentWidget = new QWidget(scrollArea);
    gridLayout = new QGridLayout(contentWidget);
    contentWidget->setLayout(gridLayout);
    // 设置滚动区域的内容
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
    // 设置主布局
    setLayout(mainLayout);

}

void HonorWallWidget::loadImagesFromDatabase()
{
    // ================== 1. 清理旧布局项 ==================
    QLayoutItem* item;
    // 循环从网格布局中取出（takeAt）所有的子项，直到取空
    while ((item = gridLayout->takeAt(0)) != nullptr) {
        // ⚠️ 隐藏坑 1：这里只是删除了 QLayoutItem 包装器
        // 真正的控件（ClickableLabel）并没有被 delete，它们仍然挂在 contentWidget 上！
        // 如果多次调用此函数，会堆积大量的隐藏控件，造成内存泄漏。
        delete item;
    }

    // ================== 2. 从数据库查询图片数据 ==================
    // 注意表名是 honor_wall，对应你之前的建表语句是 honorWall
    QSqlQuery query("SELECT id, image_data FROM honor_wall");

    while (query.next()) {
        int id = query.value(0).toInt();
        QByteArray imageData = query.value(1).toByteArray(); // 读取 BLOB 二进制数据

        // ================== 3. 二进制转图片 ==================
        QPixmap pixmap;
        // 从内存中的二进制数据解析出图片
        pixmap.loadFromData(imageData);

        if (!pixmap.isNull()) {
            // ================== 4. 动态创建自定义图片控件 ==================
            // 创建可点击的 QLabel 子类
            ClickableLabel* imageLabel = new ClickableLabel(contentWidget);

            // 对图片进行缩放，限制最大宽高（imgW, imgH），保持宽高比，且开启平滑转换保证画质
            QPixmap scaledPixmap = pixmap.scaled(imgW, imgH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            imageLabel->setPixmap(scaledPixmap);
            imageLabel->setAlignment(Qt::AlignCenter);

            // 给图片加边框和内边距，让它看起来像个相框
            imageLabel->setStyleSheet("border: 1px solid #ccc; padding: 5px;");

            // 将数据库的主键 ID 存入自定义属性，方便后续点击时区分是哪张图
            imageLabel->setProperty("id", id);


            // connect(imageLabel, &ClickableLabel::clicked, this, &HonorWallWidget::onImageClicked);

            // ================== 5. 计算行列并加入网格布局 ==================
            // 根据当前布局中已有控件的数量，动态计算应该放在第几行、第几列（每行固定 3 张）
            int row = gridLayout->count() / 3; // 行号：0, 0, 0, 1, 1, 1...
            int col = gridLayout->count() % 3; // 列号：0, 1, 2, 0, 1, 2...

            // 把控件加到网格布局的指定位置
            gridLayout->addWidget(imageLabel, row, col);
        }
        else {
            qWarning() << "无法加载图片数据！"; // 图片数据损坏时的警告
        }
    }
}

