#include "savefinalmoviedialog.h"
#include "ui_savefinalmoviedialog.h"

#include "settings.h"

#include <QFileDialog>

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

void SaveFinalMovieDialog::reset(const QString &filename, const QString &title, const QString &credits)
{
    Settings settings;

    // Title duration
    double titleScreenDuration = settings.Get("settings/titleScreenDuration").toDouble();

    // Credits duration
    double creditsDuration = settings.Get("settings/creditsDuration").toDouble();

    ui->movieSaveLocationLabel->setText(filename);

    if (titleScreenDuration > 0) {
        if (title.length() > 0) {
            ui->movieTitleLineEdit->setText(title);
        } else {
            ui->movieTitleLineEdit->clear();
            ui->movieTitleLineEdit->setPlaceholderText("Enter a title here");
        }
    } else {
        ui->movieTitleLineEdit->hide();
        ui->movieTitleLabel->hide();
    }
    if (creditsDuration > 0) {
        if (credits.length() > 0) {
            ui->creditsPlainTextEdit->setPlainText(credits);
        } else {
            ui->creditsPlainTextEdit->clear();
            ui->creditsPlainTextEdit->setPlaceholderText("Type your credits in here");
        }
    } else {
        ui->creditsPlainTextEdit->hide();
        ui->creditsLabel->hide();
    }
    adjustSize();
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
    if (newFilename.length() > 0) {
        ui->movieSaveLocationLabel->setText(newFilename);
    }
}
