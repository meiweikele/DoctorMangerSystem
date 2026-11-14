#include "doctorinfowidget.h"
#include "ui_doctorinfowidget.h"
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QFileDialog>
#include <QStandardPaths>
#include <QBuffer>
#include <QMessageBox>
#include <QSqlError>
DoctorInfoWidget::DoctorInfoWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DoctorInfoWidget)
{
    ui->setupUi(this);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(100); //垂直的表头设置为100
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

QGroupBox *DoctorInfoWidget::createFormGroup()
{
    // 1. 创建一个 QGroupBox 分组框，标题为“基本信息”
    // QGroupBox 可以看作是一个带边框和标题的容器，用来把相关控件放在一起
    QGroupBox* formGroup = new QGroupBox(tr("基本信息"));

    // 2. 创建一个表单布局（QFormLayout），并直接指定它的父对象为 formGroup
    // QFormLayout 专门用于管理“标签(Label) - 输入控件(Field)”这种两列结构的表单
    QFormLayout* formLayout = new QFormLayout(formGroup);

    //  3. 初始化所有输入控件
    // 注意：这里 new 出来的控件都没有指定父对象，因为它们会在第 4 步被布局“收养”

    // 编号输入框（单行文本）
    QLineEdit* idEdit = new QLineEdit();
    idEdit->setObjectName("idEdit"); // 设置对象名，方便后续通过 findChild 查找或设置 QSS 样式

    // 姓名输入框
    QLineEdit* nameEdit = new QLineEdit();
    nameEdit->setObjectName("nameEdit");

    // 性别下拉框
    QComboBox* genderCombo = new QComboBox();
    genderCombo->setObjectName("genderCombo");

    // 出生日期选择器，默认值为当前日期
    QDateEdit* birthdayEdit = new QDateEdit(QDate::currentDate());
    birthdayEdit->setObjectName("birthdayEdit");

    // 入学日期选择器，默认值为当前日期
    QDateEdit* joinDateEdit = new QDateEdit(QDate::currentDate());
    joinDateEdit->setObjectName("joinDateEdit");

    // 学习目标输入框
    QLineEdit* goalEdit = new QLineEdit();
    goalEdit->setObjectName("goalEdit");

    // 当前进度下拉框
    QComboBox* progressCombo = new QComboBox();
    progressCombo->setObjectName("progressCombo");

    //  4. 配置控件的具体属性

    // 给性别下拉框添加选项：男、女
    genderCombo->addItems({ tr("男"), tr("女") });

    // 给进度下拉框添加选项：0% 到 100%
    progressCombo->addItems({ tr("0%"), tr("20%"), tr("40%"), tr("60%"), tr("80%"), tr("100%") });

    // 设置日期显示的格式为 "年-月-日"（例如：2023-10-25）
    birthdayEdit->setDisplayFormat("yyyy-MM-dd");
    joinDateEdit->setDisplayFormat("yyyy-MM-dd");

    // 开启日历弹出功能（点击下拉箭头会弹出一个日历小窗口，而不是只能靠上下箭头微调）
    birthdayEdit->setCalendarPopup(true);
    joinDateEdit->setCalendarPopup(true);

    // 5. 将控件添加到表单布局中
    // addRow 方法会自动生成左侧的标签文本，并在右侧放置对应的输入控件
    formLayout->addRow(tr("编号："), idEdit);
    formLayout->addRow(tr("姓名："), nameEdit);
    formLayout->addRow(tr("性别："), genderCombo);
    formLayout->addRow(tr("出生日期："), birthdayEdit);
    formLayout->addRow(tr("入学日期："), joinDateEdit);
    formLayout->addRow(tr("学习目标："), goalEdit);
    formLayout->addRow(tr("当前进度："), progressCombo);

    // 6. 返回这个组装好的分组框，供外部（比如弹窗）添加到主布局中
    return formGroup;
}

QGroupBox *DoctorInfoWidget::createPhotoGroup()
{
    // 1. 创建一个分组框，标题为“照片上传”
    QGroupBox* photoGroup = new QGroupBox(tr("照片上传"));

    // 2. 创建一个垂直布局，并直接指定父对象为 photoGroup
    QVBoxLayout* photoLayout = new QVBoxLayout(photoGroup);

    //  3. 初始化控件
    // 用于显示照片的标签（默认没有图片）
    QLabel* lblPhotoPreview = new QLabel();
    // 用于触发选择照片的按钮
    QPushButton* btnSelectPhoto = new QPushButton(tr("选择照片"));

    //  4. 配置控件属性
    // 让标签内的图片居中显示
    lblPhotoPreview->setAlignment(Qt::AlignCenter);
    // 给预览区域一个最小尺寸，防止没图片时缩成一团，保证界面美观
    lblPhotoPreview->setMinimumSize(200, 200);
    // 固定按钮的大小
    btnSelectPhoto->setFixedSize(100, 40);

    //  5. 将控件添加到垂直布局
    // 把预览标签加进去
    photoLayout->addWidget(lblPhotoPreview);
    // 把按钮加进去，参数 0 表示不拉伸，Qt::AlignHCenter 表示按钮在水平方向居中
    photoLayout->addWidget(btnSelectPhoto, 0, Qt::AlignHCenter);

    //  6. 连接按钮点击信号
    // 这里使用了 C++11 的 Lambda 表达式来写槽函数
    // [this, lblPhotoPreview] 捕获列表：把当前类的 this 指针和标签指针捕获进来，以便在函数内部使用
    connect(btnSelectPhoto, &QPushButton::clicked, [this, lblPhotoPreview]() {

        // 弹出文件选择对话框，让用户选择图片
        QString fileName = QFileDialog::getOpenFileName(
            this,                                          // 父窗口
            tr("选择医生照片"),                             // 对话框标题
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), // 默认打开“图片”文件夹
            tr("图片文件 (*.png *.jpg *.jpeg)")            // 文件过滤，只显示图片格式
            );

        // 如果用户选择了文件（不是点击了取消）
        if (!fileName.isEmpty()) {
            // 尝试从硬盘加载图片
            QPixmap pixmap(fileName);

            // 检查图片是否加载成功
            if (!pixmap.isNull()) {
                // --- 智能缩放图片 ---
                // 将图片按比例缩放，使其适应预览标签的大小（留出 30 像素的边距）
                pixmap = pixmap.scaled(
                    lblPhotoPreview->width() - 30,
                    lblPhotoPreview->height() - 30,
                    Qt::KeepAspectRatio // 保持宽高比，图片不会变形
                    );
                // 将缩放后的图片显示在标签上
                lblPhotoPreview->setPixmap(pixmap);

                // --- 转换为字节数组（为存入数据库做准备） ---
                QBuffer buffer(&photoData);
                buffer.open(QIODevice::WriteOnly); // 以只写方式打开缓冲区
                pixmap.save(&buffer, "PNG");       // 将图片以 PNG 格式保存到字节数组中
            }
            else {
                // 如果文件损坏或不是有效图片
                QMessageBox::warning(this, tr("错误"), tr("无法加载图片文件！"));
            }
        }
    });

    // 7. 返回组装好的分组框
    return photoGroup;
}

void DoctorInfoWidget::handleDialogAccepted(QGroupBox *formGroup, QGroupBox *photoGroup)
{
    // 1. 获取表单数据
    // 因为 formGroup 是动态创建并返回的，外部拿不到里面的局部变量
    // 所以通过 objectName（之前用 setObjectName 设置的）来“揪出”这些控件
    QLineEdit* idEdit = formGroup->findChild<QLineEdit*>("idEdit");
    QLineEdit* nameEdit = formGroup->findChild<QLineEdit*>("nameEdit");
    QComboBox* genderCombo = formGroup->findChild<QComboBox*>("genderCombo");
    QDateEdit* birthdayEdit = formGroup->findChild<QDateEdit*>("birthdayEdit");
    QDateEdit* joinDateEdit = formGroup->findChild<QDateEdit*>("joinDateEdit");
    QLineEdit* goalEdit = formGroup->findChild<QLineEdit*>("goalEdit");
    QComboBox* progressCombo = formGroup->findChild<QComboBox*>("progressCombo");

    // 2. 数据校验
    // 检查必填项：如果学号或姓名为空，弹出警告并直接退出函数（不执行插入）
    if (idEdit->text().isEmpty() || nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("学号和姓名不能为空！"));
        return;
    }

    // 3. 检查学号唯一性
    // 为了防止重复插入同一个学号，先查询数据库看是否存在
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT id FROM DoctorInfo WHERE id = ?");
    checkQuery.addBindValue(idEdit->text());

    // 如果查询成功，并且有结果（next() 返回 true），说明学号已存在
    if (checkQuery.exec() && checkQuery.next()) {
        QMessageBox::warning(this, tr("错误"), tr("学号 %1 已存在！").arg(idEdit->text()));
        return;
    }

    //  4. 插入数据到数据库
    // 开启数据库事务：确保后面的插入操作要么全部成功，要么全部失败（防止数据写到一半断电）
    QSqlDatabase::database().transaction();

    QSqlQuery insertQuery;
    if (!insertQuery.prepare(
            "INSERT INTO DoctorInfo "
            "(id, name, gender, birthday, join_date, study_goal, progress, photo) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
            )) {
        qDebug() << "准备SQL失败，真实原因:" << insertQuery.lastError().text();
        QMessageBox::critical(this, "错误", insertQuery.lastError().text());
        return;
    }

    // 5. 绑定参数（防 SQL 注入）
    // 使用 addBindValue 而不是直接拼接字符串，是防止 SQL 注入的标准做法
    insertQuery.addBindValue(idEdit->text());
    insertQuery.addBindValue(nameEdit->text());
    insertQuery.addBindValue(genderCombo->currentText());
    // 日期转换为指定格式的字符串存入数据库
    insertQuery.addBindValue(birthdayEdit->date().toString("yyyy-MM-dd"));
    insertQuery.addBindValue(joinDateEdit->date().toString("yyyy-MM-dd"));
    insertQuery.addBindValue(goalEdit->text());
    insertQuery.addBindValue(progressCombo->currentText());

    // 【重点】处理照片数据：
    // 如果 photoData 为空（用户没选照片），就绑定一个空的 QVariant（对应数据库的 NULL）
    // 否则绑定二进制的图片数据
    insertQuery.addBindValue(photoData.isEmpty() ? QVariant() : photoData);

    //  6. 执行插入并处理结果
    if (!insertQuery.exec()) {
        // 如果插入失败，回滚事务（撤销之前的操作），并弹出详细错误信息
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, tr("错误"), tr("添加失败：") + insertQuery.lastError().text());
    }
    else {
        // 如果插入成功，提交事务（永久保存数据）
        QSqlDatabase::database().commit();

        // 刷新表格界面，让最新数据立刻显示出来
        refreshTable();

        // 弹出成功提示框
        QMessageBox::information(this, tr("成功"), tr("已成功添加医生：%1").arg(nameEdit->text()));
    }
}
void DoctorInfoWidget::on_btnAdd_clicked()
{
    // 1. 在当前窗口（this）上创建一个局部模态对话框
    // dlg 分配在栈上，exec() 结束后会自动析构释放内存
    QDialog dlg(this);
    dlg.setWindowTitle(tr("添加医生信息")); // 设置弹窗标题
    dlg.setMinimumSize(600, 400);          // 设置弹窗最小尺寸

    //2. 初始化对话框布局
    // 创建主垂直布局，并直接指定其父对象为 &dlg
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);

    // 创建水平布局，用于左右并排显示表单和照片区域
    QHBoxLayout* contentLayout = new QHBoxLayout();
    mainLayout->addLayout(contentLayout); // 将水平布局加入到主布局中

    // 3. 添加表单和照片区域
    // 调用外部辅助函数创建两个分组框（GroupBox）
    QGroupBox* formGroup = createFormGroup();   // 左侧：表单输入区
    QGroupBox* photoGroup = createPhotoGroup(); // 右侧：照片显示/上传区

    // 将这两个分组框加入到水平布局中，参数 '1' 表示拉伸因子为1，即两者平分水平空间
    contentLayout->addWidget(formGroup, 1);
    contentLayout->addWidget(photoGroup, 1);

    // 4. 添加底部按钮区域
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnConfirm = new QPushButton(tr("确认"));
    QPushButton* btnCancel = new QPushButton(tr("取消"));

    // 配置按钮宽度
    btnConfirm->setFixedWidth(150);
    btnCancel->setFixedWidth(150);

    // 将按钮添加到水平布局
    btnLayout->addStretch();              // 左侧加弹簧，把按钮往中间挤
    btnLayout->addWidget(btnConfirm);     // 添加确认按钮
    btnLayout->addWidget(btnCancel);      // 添加取消按钮
    btnLayout->addStretch();              // 右侧加弹簧，把按钮往中间挤

    // 将按钮布局加入到主垂直布局的最下方
    mainLayout->addLayout(btnLayout);

    //  5. 连接按钮信号
    // 点击“确认”按钮，触发对话框的 accept() 槽函数（返回 QDialog::Accepted）
    connect(btnConfirm, &QPushButton::clicked, &dlg, &QDialog::accept);
    // 点击“取消”按钮，触发对话框的 reject() 槽函数（返回 QDialog::Rejected）
    connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    // 6. 执行对话框并处理结果
    // dlg.exec() 会阻塞程序，直到用户点击“确认”或“取消”关闭窗口
    if (dlg.exec() == QDialog::Accepted)
    {
        // 如果用户点击了“确认”，则调用处理函数，将表单和照片里的数据提取出来保存
        handleDialogAccepted(formGroup, photoGroup);
    }
}

