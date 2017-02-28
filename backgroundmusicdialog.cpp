#include "backgroundmusicdialog.h"
#include "ui_backgroundmusicdialog.h"

#include <QFileDialog>
#include <QAudioDecoder>
#include <QSettings>
#include "settingsdialog.h"

#include <iostream>

#include "utils.h"

BackgroundMusicDialog::BackgroundMusicDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BackgroundMusicDialog),
    _decoder(NULL)
{
    ui->setupUi(this);
    ui->playPauseButton->setText("");
    ui->rewindButton->setText("");
    ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
    ui->rewindButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaSkipBackward));
}

BackgroundMusicDialog::~BackgroundMusicDialog()
{
    delete ui;
}

void BackgroundMusicDialog::on_chooseMusicFileButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open Bacgkround Music File"), "Music files", tr("Sound files (*.mp3;*.wav);;All files (*.*)"));
    loadFile(filename);
}

void BackgroundMusicDialog::loadFile (const QString &filename)
{
    QAudioFormat desiredFormat;
    desiredFormat.setChannelCount(2);
    desiredFormat.setCodec("audio/x-raw");
    desiredFormat.setSampleType(QAudioFormat::UnSignedInt);
    desiredFormat.setSampleRate(48000);
    desiredFormat.setSampleSize(16);

    if (!_decoder) {
        _decoder = new QAudioDecoder(this);
    }
    _decoder->setAudioFormat(desiredFormat);
    _decoder->setSourceFilename(filename);
    ui->waveform->reset();

    QObject::connect (_decoder, &QAudioDecoder::bufferReady, this, &BackgroundMusicDialog::readBuffer);
    _decoder->start();
}


void BackgroundMusicDialog::readBuffer ()
{
    QAudioBuffer buffer = _decoder->read();
    ui->waveform->setDuration(_decoder->duration());
    ui->waveform->addBuffer(buffer);

    QSettings settings;
    double preTitleDuration = settings.value("settings/preTitleScreenDuration",
                                             SettingsDialog::getDefault("settings/preTitleScreenDuration")).toDouble();
    double titleDuration = settings.value("settings/titleScreenDuration",
                                             SettingsDialog::getDefault("settings/titleScreenDuration")).toDouble();
    double creditsDuration = settings.value("settings/creditsDuration",
                                             SettingsDialog::getDefault("settings/creditsDuration")).toDouble();
    double movieDuration = _movieDuration;

    double totalDuration = preTitleDuration + titleDuration + movieDuration + creditsDuration;

    ui->waveform->setSelectionLength(totalDuration*1000);
}

void BackgroundMusicDialog::setMovieDuration (double duration)
{
    _movieDuration = duration;
}

