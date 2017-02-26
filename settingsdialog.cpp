#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include <QImageWriter>
#include <QCameraInfo>
#include <QFileDialog>
#include <QCloseEvent>
#include <QMessageBox>

QMap<QString, QVariant> SettingsDialog::SETTING_DEFAULTS;

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    this->setWindowModality(Qt::WindowModal);

    // Set up the widgets by getting lists of file formats, cameras, and camera data

    // File formats:
    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    foreach (const QByteArray &format, formats) {
        QString formatString (format);
        // Right now our camera will only output JPG, so only list that one.
        if (formatString.toUpper() == "JPG") {
            ui->fileTypeCombo->addItem(formatString.toUpper());
        }
    }

    // Frames per second
    ui->fpsCombo->addItem(QString("10"), QVariant(10));
    ui->fpsCombo->addItem(QString("15"), QVariant(15));
    ui->fpsCombo->addItem(QString("20"), QVariant(20));
    ui->fpsCombo->addItem(QString("25"), QVariant(25));
    ui->fpsCombo->addItem(QString("30"), QVariant(30));

    // Camera
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    ui->cameraCombo->addItem ("Always use default");
    ui->cameraCombo->insertSeparator(1);
    foreach (const QCameraInfo &camera, cameras) {
        ui->cameraCombo->addItem (camera.description());
    }

    // Resolution
    ui->resolutionCombo->addItem(QString("640 x 480 (Default)"), QVariant(QSize(640,480)));
    //ui->resolutionCombo->addItem(QString("800 x 600"), QVariant(QSize(800,600)));
    //ui->resolutionCombo->addItem(QString("1280 x 720 (Widescreen)"), QVariant(QSize(1280,720)));
    //ui->resolutionCombo->addItem(QString("1920 x 1080 (Widescreen)"), QVariant(QSize(1920,1080)));
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::load ()
{
    QSettings settings;

    // File type
    QString imageFileType = settings.value("settings/imageFileType",getDefault("settings/imageFileType")).toString();
    int fileTypeIndex = ui->fileTypeCombo->findText(imageFileType.toUpper());
    if (fileTypeIndex != -1) {
        ui->fileTypeCombo->setCurrentIndex (fileTypeIndex);
    } else {
        ui->fileTypeCombo->setCurrentText("JPG");
    }

    // Frames per second
    qint32 framesPerSecond = settings.value("settings/framesPerSecond",getDefault("settings/framesPerSecond")).toInt();
    int fpsIndex = ui->fpsCombo->findData(framesPerSecond);
    if (fpsIndex > -1) {
        ui->fpsCombo->setCurrentIndex(fpsIndex);
    } else {
        ui->fpsCombo->setCurrentText(QString::number(15));
    }

    // Image storage location
    QString imageStorageLocation = settings.value("settings/imageStorageLocation",getDefault("settings/imageStorageLocation")).toString();
    ui->imageLocationLineEdit->setText(imageStorageLocation);

    // Image filename format
    QString imageFilenameFormat = settings.value("settings/imageFilenameFormat",getDefault("settings/imageFilenameFormat")).toString();
    ui->filenameFormatLineEdit->setText(imageFilenameFormat);

    // Camera (what is stored is the description string from the QCamera object)
    QString cameraString = settings.value("settings/camera",getDefault("settings/camera")).toString();
    int cameraIndex = ui->fileTypeCombo->findText(cameraString.toUpper());
    if (cameraIndex != -1) {
        ui->fileTypeCombo->setCurrentIndex (cameraIndex);
    } else {
        ui->fileTypeCombo->setCurrentText("Always use default");
    }

    // Resolution
    QSize resolution = settings.value("settings/resolution",getDefault("settings/resolution")).toSize();
    int resolutionIndex = ui->resolutionCombo->findData (resolution);
    if (resolutionIndex > -1) {
        ui->resolutionCombo->setCurrentIndex(resolutionIndex);
    } else {
        ui->resolutionCombo->setCurrentIndex(0);
    }

    // Pre-title screen
    QString titleScreenLocation = settings.value("settings/preTitleScreenLocation",getDefault("settings/preTitleScreenLocation")).toString();
    ui->preTitleScreenFileLineEdit->setText(titleScreenLocation);

    // Pre-title duration
    double preTitleScreenDuration = settings.value("settings/preTitleScreenDuration",getDefault("settings/preTitleScreenDuration")).toDouble();
    ui->preTitleDurationSpinbox->setValue(preTitleScreenDuration);

    // Title duration
    double titleScreenDuration = settings.value("settings/titleScreenDuration",getDefault("settings/titleScreenDuration")).toDouble();
    ui->titleDurationSpinbox->setValue(titleScreenDuration);

    // Credits duration
    double creditsDuration = settings.value("settings/creditsDuration",getDefault("settings/creditsDuration")).toDouble();
    ui->creditsDurationSpinbox->setValue(creditsDuration);
}

void SettingsDialog::store ()
{
    QSettings settings;
    const QString prefix ("settings/");

    // File type
    QString fileType = ui->fileTypeCombo->currentText();
    settings.setValue (prefix + "imageFileType", fileType);

    // Frames per second
    int fps = ui->fpsCombo->currentText().toInt();
    settings.setValue (prefix + "framesPerSecond", fps);

    // Image storage location
    QString imageStorageLocation = ui->imageLocationLineEdit->text();
    if (imageStorageLocation == "") {
        imageStorageLocation = "./";
    }
    settings.setValue(prefix + "imageStorageLocation", imageStorageLocation);

    // Image filename format
    QString imageFilenameFormat = ui->filenameFormatLineEdit->text();
    settings.setValue(prefix + "imageFilenameFormat", imageFilenameFormat);

    // Camera
    QString camera = ui->cameraCombo->currentText();
    settings.setValue(prefix + "camera", camera);

    // Resolution
    QSize resolution = ui->resolutionCombo->currentData().toSize();
    settings.setValue(prefix + "resolution", resolution);


    // Pre-title screen
    QString titleScreenLocation = ui->preTitleScreenFileLineEdit->text();
    settings.setValue("settings/preTitleScreenLocation", titleScreenLocation);

    // Pre-title duration
    double preTitleScreenDuration = ui->preTitleDurationSpinbox->value();
    settings.setValue("settings/preTitleScreenDuration", preTitleScreenDuration);

    // Title duration
    double titleScreenDuration = ui->titleDurationSpinbox->value();
    settings.setValue("settings/titleScreenDuration", titleScreenDuration);

    // Credits duration
    double creditsDuration = ui->creditsDurationSpinbox->value();
    settings.setValue("settings/creditsDuration", creditsDuration);
}

void SettingsDialog::on_imageLocationBrowseButton_clicked()
{
    // Show the folder selection dialog
    QString imageLocation = QFileDialog::getExistingDirectory(this, tr("Choose base location for images"));
    if (imageLocation != QString::null) {
        ui->imageLocationLineEdit->setText(imageLocation);
    }
}

void SettingsDialog::on_filenameFormatHelpButton_clicked()
{
    _filenameConstructionHelp.show();
}

void SettingsDialog::on_preTitleScreenFileBrowseButton_clicked()
{
    // Show the folder selection dialog
    QString imageLocation = QFileDialog::getOpenFileName(this, tr("Select the image file to use as the fre-title screen"),"","Image files (*.jpg *.png *.bmp *.gif);;All files (*.*)");

    if (imageLocation != QString::null) {
        ui->preTitleScreenFileLineEdit->setText(imageLocation);
    }
}

void SettingsDialog::closeEvent(QCloseEvent *event)
{
    _filenameConstructionHelp.hide();
    event->accept();
}

void SettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QSettings settings;
    if (button->text() == "Reset") {
        QMessageBox::StandardButton result = QMessageBox::warning (this, "Confirm reset", "This will reset all settings to their default values, and cannot be undone. Do you want to reset?", QMessageBox::Yes|QMessageBox::Cancel);
        if (result == QMessageBox::Yes) {
            for (auto i = SETTING_DEFAULTS.constBegin(); i != SETTING_DEFAULTS.constEnd(); ++i) {
                settings.setValue (i.key(), i.value());
            }
            load();
        }
    }
}


QVariant SettingsDialog::getDefault (const QString &key)
{
    // This lets us set up all the defaults in one concise location
    if (SettingsDialog::SETTING_DEFAULTS.empty()) {
        SETTING_DEFAULTS.insert("settings/imageFileType","JPG");
        SETTING_DEFAULTS.insert("settings/framesPerSecond",15);
        SETTING_DEFAULTS.insert("settings/imageStorageLocation","Image Files/");
        SETTING_DEFAULTS.insert("settings/imageFilenameFormat","yyyy-MM-dd-hh-mm-ss");
        SETTING_DEFAULTS.insert("settings/camera","Always use default");
        SETTING_DEFAULTS.insert("settings/resolution",QSize(640,480));
        SETTING_DEFAULTS.insert("settings/preTitleScreenLocation","./PreTitleScreen.jpg");
        SETTING_DEFAULTS.insert("settings/preTitleScreenDuration",2.0);
        SETTING_DEFAULTS.insert("settings/titleScreenDuration",2.0);
        SETTING_DEFAULTS.insert("settings/creditsDuration",5.0);
    }
    return SETTING_DEFAULTS[key];
}
