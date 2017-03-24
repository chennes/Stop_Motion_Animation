#include "backgroundmusicdialog.h"
#include "ui_backgroundmusicdialog.h"

#include <QFileDialog>
#include <QAudioDecoder>
#include "settings.h"
#include "settingsdialog.h"

#include <iostream>

#include "utils.h"

BackgroundMusicDialog::BackgroundMusicDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BackgroundMusicDialog),
    _decoder(NULL),
    _musicSet (false)
{
    ui->setupUi(this);
    ui->playPauseButton->setText("");
    ui->rewindButton->setText("");
    ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
    ui->rewindButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaSkipBackward));

    connect (this, &BackgroundMusicDialog::finished, this, &BackgroundMusicDialog::dialogClosed);

    _player = new QMediaPlayer (this);
    _player->setNotifyInterval(75);
    connect(_player, &QMediaPlayer::positionChanged, this, &BackgroundMusicDialog::playerPositionChanged);
    connect(_player, &QMediaPlayer::stateChanged, this, &BackgroundMusicDialog::playerStateChanged);
    connect(ui->waveform, &Waveform::playheadManuallyChanged, this, &BackgroundMusicDialog::setPlayhead);
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
    // Two different loads to do: the player and the waveform. The player is just simple
    // Qt, do it first:
    _filename = filename;
    _player->setMedia(QUrl::fromLocalFile(filename));
    _player->setVolume(50);

    // Now the hard part: load the data out of the file
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
    QObject::connect (_decoder, &QAudioDecoder::finished, this, &BackgroundMusicDialog::readFinished);
    _decoder->start();

    ui->removeMusicButton->setDisabled(false);
    ui->playPauseButton->setDisabled(false);
    ui->rewindButton->setDisabled(false);
}


void BackgroundMusicDialog::readBuffer ()
{
    QAudioBuffer buffer = _decoder->read();
    ui->waveform->setDuration(_decoder->duration());
    ui->waveform->addBuffer(buffer);
}

void BackgroundMusicDialog::readFinished ()
{
    Settings settings;
    double preTitleDuration = settings.Get("settings/preTitleScreenDuration").toDouble();
    double titleDuration = settings.Get("settings/titleScreenDuration").toDouble();
    double creditsDuration = settings.Get("settings/creditsDuration").toDouble();
    double movieDuration = _movieDuration;

    double totalDuration = preTitleDuration + titleDuration + movieDuration + creditsDuration;

    ui->waveform->ClearRegions();
    ui->waveform->setDuration(_decoder->duration());
    ui->waveform->setSelectionLength(totalDuration*1000);
    qint64 offset = 0;
    ui->waveform->AddRegion("Intro", offset, offset+preTitleDuration*1000, Qt::black);
    offset += preTitleDuration*1000;
    ui->waveform->AddRegion("Title", offset, offset+titleDuration*1000, Qt::black);
    offset += titleDuration*1000;
    ui->waveform->AddRegion("Movie", offset, offset+movieDuration*1000, Qt::blue);
    offset += movieDuration*1000;
    ui->waveform->AddRegion("Credits", offset, offset+creditsDuration*1000, Qt::black);

    _decoder->disconnect();
    _musicSet = true;
}

void BackgroundMusicDialog::setMovieDuration (double duration)
{
    _movieDuration = duration;
    if (_musicSet) {
        // Trigger a re-creation of the regions
        readFinished ();
    }
}


void BackgroundMusicDialog::on_playPauseButton_clicked()
{
    if (_player->state() == QMediaPlayer::StoppedState ||
        _player->state() == QMediaPlayer::PausedState) {
        _player->setPosition(ui->waveform->getPlayheadPosition());
        on_volumeSlider_valueChanged (ui->volumeSlider->value());
        _player->play();
    } else {
        ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
        _player->pause();
    }
}


void BackgroundMusicDialog::playerPositionChanged (qint64 newPosition)
{
    ui->waveform->setPlayheadPosition(newPosition);
}

void BackgroundMusicDialog::playerStateChanged (QMediaPlayer::State state)
{
    switch (state) {
    case QMediaPlayer::StoppedState:
        // This only happens at the end of the track. Play it again.
        _player->play();
        break;
    case QMediaPlayer::PausedState:
        ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
        break;
    case QMediaPlayer::PlayingState:
        ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPause));
        break;
    }
}

void BackgroundMusicDialog::setPlayhead (qint64 newPosition)
{
    if (_player->state() == QMediaPlayer::PlayingState) {
        _player->setPosition(newPosition);
    }
}

void BackgroundMusicDialog::showEvent(QShowEvent *)
{
    if (!_musicSet) {
        on_chooseMusicFileButton_clicked();
    }
}

void BackgroundMusicDialog::dialogClosed(int)
{
    _player->pause();
}

void BackgroundMusicDialog::on_removeMusicButton_clicked()
{
    ui->removeMusicButton->setDisabled(true);
    ui->playPauseButton->setDisabled(true);
    ui->rewindButton->setDisabled(true);
    ui->waveform->reset();
    _musicSet = false;
}

void BackgroundMusicDialog::on_rewindButton_clicked()
{
    _player->setPosition(0);
    ui->waveform->setPlayheadPosition(0);
}

SoundEffect BackgroundMusicDialog::getBackgroundMusic () const
{
    if (_musicSet) {
        double in = (double)ui->waveform->getSelectionStart() / 1000.0;
        double out = in + (double)ui->waveform->getSelectionLength() / 1000.0;
        double linearVolume = QAudio::convertVolume(ui->volumeSlider->value() / qreal(200.0),
                                                      QAudio::LogarithmicVolumeScale,
                                                      QAudio::LinearVolumeScale);
        return SoundEffect (_filename, 0.0, in, out, linearVolume);
    } else {
        return SoundEffect ();
    }
}

void BackgroundMusicDialog::on_volumeSlider_valueChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(200.0),
                                                  QAudio::LogarithmicVolumeScale,
                                                  QAudio::LinearVolumeScale);
    _player->setVolume (qRound(linearVolume * 100));
}
