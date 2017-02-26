#include "savefinalmoviedialog.h"
#include "ui_savefinalmoviedialog.h"

#include <QFileDialog>
#include <QSettings>

SaveFinalMovieDialog::SaveFinalMovieDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SaveFinalMovieDialog)
{
    ui->setupUi(this);
}

SaveFinalMovieDialog::~SaveFinalMovieDialog()
{
    delete ui;
}

void SaveFinalMovieDialog::reset(const QString &name)
{
    ui->movieSaveLocationLabel->setText(name);
    ui->creditsPlainTextEdit->clear();
    QSettings settings;
    QString imageStorageLocation = settings.value("settings/imageStorageLocation","Image Files/").toString();
    ui->movieSaveLocationLabel->setText(imageStorageLocation+"/" + name + ".mp4");
}

QString SaveFinalMovieDialog::filename() const
{
    return ui->movieSaveLocationLabel->text();
}

QString SaveFinalMovieDialog::movieTitle() const
{
    return ui->movieTitleLineEdit->text();
}

QString SaveFinalMovieDialog::credits() const
{
    return ui->creditsPlainTextEdit->toPlainText();
}

void SaveFinalMovieDialog::on_changeLocationButton_clicked()
{
    QString newFilename = QFileDialog::getSaveFileName(this, "Save movie to...", "", "Movie files (*.mp4);;All files (*.*)");
    if (newFilename != QString::null) {
        ui->movieSaveLocationLabel->setText(newFilename);
    }
}
