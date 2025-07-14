#include "CreateTagDialog.h"
#include "ui_CreateTagDialog.h"
#include <QFileDialog>
#include <QPixmap>

CreateTagDialog::CreateTagDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CreateTagDialog)
{
    ui->setupUi(this);
    connect(ui->buttonSelectImage, &QPushButton::clicked, this, &CreateTagDialog::onSelectImageClicked);
    connect(ui->buttonBoxOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->buttonBoxCancel, &QPushButton::clicked, this, &QDialog::reject);
    updateImagePreview();
}

CreateTagDialog::~CreateTagDialog()
{
    delete ui;
}

QString CreateTagDialog::getTagName() const
{
    return ui->lineEditTagName->text().trimmed();
}

QString CreateTagDialog::getTagImagePath() const
{
    return m_imagePath;
}

void CreateTagDialog::onSelectImageClicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("选择标签图片"), QString(), tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif *.svg *.ico);;所有文件 (*.*)"));
    if (!file.isEmpty()) {
        m_imagePath = file;
        updateImagePreview();
    }
}

void CreateTagDialog::updateImagePreview()
{
    if (!m_imagePath.isEmpty()) {
        QPixmap pix(m_imagePath);
        ui->labelImagePreview->setPixmap(pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        ui->labelImagePreview->setPixmap(QPixmap());
    }
}

void CreateTagDialog::setTagName(const QString& name)
{
    ui->lineEditTagName->setText(name);
}

void CreateTagDialog::setImagePath(const QString& path)
{
    m_imagePath = path;
    updateImagePreview();
}