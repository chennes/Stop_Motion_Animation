#include "backgroundmusicdialog.h"
#include "ui_backgroundmusicdialog.h"

#include <QFileDialog>
#include <QAudioDecoder>
#include <QSettings>

#include <iostream>

#include "utils.h"

BackgroundMusicDialog::BackgroundMusicDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BackgroundMusicDialog),
    _decoder(NULL)
{
    ui->setupUi(this);
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
}

