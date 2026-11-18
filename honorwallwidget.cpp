#include "honorwallwidget.h"
#include "ui_honorwallwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include <QGridLayout>
#include <QSqlQuery>
#include <QFileDialog>
#include <QMessageBox>
#include <QBuffer>
#include <QSqlError>
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
    // 打开文件对话框选择图片
    QString imagePath = QFileDialog::getOpenFileName(this, "选择图片", "", "图片文件 (*.png *.jpg *.jpeg *.bmp)");
    if (!imagePath.isEmpty()) addImageToWall(imagePath);

}

void HonorWallWidget::modifyImage()
{
    // ================== 1. 检查是否选中了图片 ==================
    if (!selectedLabel) {
        QMessageBox::warning(this, "错误", "请先选择一张图片！");
        return;
    }

    // ================== 2. 弹出文件选择对话框 ==================
    QString imagePath = QFileDialog::getOpenFileName(
        this, "选择图片", "", "图片文件 (*.png *.jpg *.jpeg *.bmp)"
        );
    // 如果用户点了取消，直接退出
    if (imagePath.isEmpty()) return;

    // ================== 3. 加载新图片 ==================
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "错误", "无法加载图片！");
        return;
    }

    // ================== 4. 将新图片转为二进制数据 ==================
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG"); // 统一转成 PNG 二进制流，便于存入数据库

    // ================== 5. 获取选中图片的数据库 ID ==================
    // 这里能拿到 id，全靠之前 loadImagesFromDatabase 里 setProperty("id", id) 的功劳
    int id = selectedLabel->property("id").toInt();

    // ================== 6. 更新数据库 ==================
    QSqlQuery query;
    query.prepare("UPDATE honorWall SET image_data = :image_data WHERE id = :id");
    query.bindValue(":image_data", imageData);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "更新数据失败：" << query.lastError().text();
        QMessageBox::warning(this, "错误", "修改图片失败：" + query.lastError().text());
        return;
    }

    // ================== 7. 更新界面上的图片 ==================
    // 直接在当前选中的 label 上替换 pixmap 即可，不需要重建整个网格
    QPixmap scaledPixmap = pixmap.scaled(imgW, imgH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    selectedLabel->setPixmap(scaledPixmap);

    // 💡 建议补充：修改成功后，弹出一个提示，或者将 selectedLabel 置空
     QMessageBox::information(this, "成功", "图片已成功替换！");
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
    // while ((item = gridLayout->takeAt(0)) != nullptr) {
    //     // ⚠️ 隐藏坑 1：这里只是删除了 QLayoutItem 包装器
    //     // 真正的控件（ClickableLabel）并没有被 delete，它们仍然挂在 contentWidget 上！
    //     // 如果多次调用此函数，会堆积大量的隐藏控件，造成内存泄漏。
    //     delete item;
    // }

    while ((item = gridLayout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater(); // 安全地销毁真正的图片控件
        }
        delete item;
    }

    // ================== 2. 从数据库查询图片数据 ==================
    QSqlQuery query("SELECT id, image_data FROM honorWall");

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


            connect(imageLabel, &ClickableLabel::clicked, this, &HonorWallWidget::onImageClicked);

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

// ==========================================================
// 1. 将本地图片存入数据库 (addImageToWall)
// ==========================================================
void HonorWallWidget::addImageToWall(const QString &imagePath)
{
    // ================== 1. 加载本地图片文件 ==================
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "错误", "无法加载图片！");
        return;
    }

    // ================== 2. 将图片转为二进制 (BLOB) ==================
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG"); // 以 PNG 格式写入缓冲区

    // ================== 3. 插入数据库 ==================
    QSqlQuery query;
    // 注意：表格名是 honorWall（注意大小写）
    query.prepare("INSERT INTO honorWall (image_data, description, added_date) VALUES(:image_data, :description, :added_date)");
    query.bindValue(":image_data", imageData);
    query.bindValue(":description", "未填写描述");
    query.bindValue(":added_date", QDate::currentDate().toString());

    if (!query.exec()) {
        qWarning() << "插入数据失败：" << query.lastError().text();
        return;
    }

    // ================== 4. 更新界面 ==================
    // ⚠️ 逻辑 Bug 1：这里是直接调用了 addImageToUI 往最后面追加。
    // 如果你此时刚好由于某次查询，界面上只有部分图片（比如搜索了某天的图片），
    // 直接往末尾加，会导致界面顺序和数据库顺序彻底错乱。
    addImageToUI(pixmap);
}

// ==========================================================
// 2. 在界面上动态生成图片控件 (addImageToUI)
// ==========================================================
void HonorWallWidget::addImageToUI(const QPixmap& pixmap)
{
    if (pixmap.isNull()) {
        qWarning() << "图片无效！";
        return;
    }

    // ================== 1. 缩放图片 ==================
    QPixmap scaledPixmap = pixmap.scaled(imgW, imgH, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // ================== 2. 创建控件 ==================
    ClickableLabel* imageLabel = new ClickableLabel(contentWidget);
    if (!imageLabel) {
        qWarning() << "无法创建 QLabel！";
        return;
    }

    // ================== 3. 配置控件属性 ==================
    imageLabel->setPixmap(scaledPixmap);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("border: 1px solid #ccc; padding: 5px;");

    // ⚠️ 逻辑 Bug 2：这里没有给 imageLabel 设置 property("id")！
    // 导致你在 loadImagesFromDatabase() 里点击图片时，无法获取这个新图片的数据库 ID。
    // 应该在这里把刚插入数据库的自增 ID 拿过来，设置进去。

    connect(imageLabel, &ClickableLabel::clicked, this, &HonorWallWidget::onImageClicked);

    // ================== 4. 添加到网格布局 ==================
    // ⚠️ 逻辑 Bug 3（极其容易踩的坑）：
    // gridLayout->count() 返回的是当前所有“子项”的数量，但有时候占位符也算在内。
    // 如果此时你刚好清空过布局，或者有其他的控件，这里的计算会完全错位，图片可能重叠在一起。
    int row = gridLayout->count() / 3;
    int col = gridLayout->count() % 3;
    gridLayout->addWidget(imageLabel, row, col);
}

void HonorWallWidget::onImageClicked()
{
    if (selectedLabel) {// 取消之前选中的图片样式
        selectedLabel->setStyleSheet("border: 1px solid #ccc; padding: 5px;");
    }
    selectedLabel = qobject_cast<ClickableLabel*>(sender());
    if (selectedLabel) {// 更新选中的图片
        selectedLabel->setStyleSheet("border: 2px solid red; padding: 5px;");
    }
}

void HonorWallWidget::deleteImage()
{
    // ================== 1. 检查是否选中了图片 ==================
    if (!selectedLabel) {
        QMessageBox::warning(this, "错误", "请先选择一张图片！");
        return;
    }

    // ================== 2. 弹出确认对话框 ==================
    if (QMessageBox::question(this, "确认删除", "确定要删除这张图片吗？") != QMessageBox::Yes) return;

    // ================== 3. 获取图片对应的数据库 ID ==================
    // 这里的 property("id") 之所以能拿到值，是因为我们在 loadImagesFromDatabase 里正确绑定了它
    int id = selectedLabel->property("id").toInt();

    // ================== 4. 从数据库删除记录 ==================
    QSqlQuery query;
    query.prepare("DELETE FROM honorWall WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "删除数据失败：" << query.lastError().text();
        return;
    }

    // ================== 5. 从界面删除图片控件 ==================
    gridLayout->removeWidget(selectedLabel); // 先从布局中摘除
    delete selectedLabel;                    // 再彻底销毁控件（释放内存）
    selectedLabel = nullptr;                 // 防止野指针（非常规范的做法！）

    // ================== 6. 重新排列剩余图片 ==================
    reorderImages();
}

void HonorWallWidget::reorderImages()
{
    // ================== 1. 清空布局中的所有内容 ==================
    QLayoutItem* item;
    while ((item = gridLayout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater(); // 安全地销毁真正的图片控件
        }
        delete item;
    }

    // ================== 2. 重新加载图片 ==================
    // 从数据库重新拉取全部数据，重新填充到网格布局中
    loadImagesFromDatabase();
}


