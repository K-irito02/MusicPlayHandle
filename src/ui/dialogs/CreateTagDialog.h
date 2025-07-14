#ifndef CREATETAGDIALOG_H
#define CREATETAGDIALOG_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class CreateTagDialog;
}
QT_END_NAMESPACE

class CreateTagDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreateTagDialog(QWidget *parent = nullptr);
    ~CreateTagDialog();

    QString getTagName() const;
    QString getTagImagePath() const;
    
    // 设置方法，用于编辑标签
    void setTagName(const QString& name);
    void setImagePath(const QString& path);

private slots:
    void onSelectImageClicked();

private:
    Ui::CreateTagDialog *ui;
    QString m_imagePath;
    void updateImagePreview();
};

#endif // CREATETAGDIALOG_H