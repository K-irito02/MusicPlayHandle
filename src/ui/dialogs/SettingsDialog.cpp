#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include <QSettings>
#include <QDialogButtonBox>

void SettingsDialog::saveSettings()
{
    QSettings settings;
    settings.setValue("theme", ui->comboBoxTheme->currentIndex());
    settings.setValue("language", ui->comboBoxLanguage->currentIndex());
    settings.setValue("defaultVolume", ui->sliderVolume->value());
    settings.sync();
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    // 加载已有设置
    QSettings settings;
    ui->comboBoxTheme->setCurrentIndex(settings.value("theme", 0).toInt());
    ui->comboBoxLanguage->setCurrentIndex(settings.value("language", 0).toInt());
    ui->sliderVolume->setValue(settings.value("defaultVolume", 50).toInt());
    // 连接按钮
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        emit settingsChanged();
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}