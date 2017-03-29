#include "soundselectiondialog.h"
#include "ui_soundselectiondialog.h"

#include <QFileDialog>
#include <QAudioDecoder>
#include "settings.h"
#include "settingsdialog.h"

#include <iostream>

#include "utils.h"
#include "multiregionwaveform.h"
#include "variableselectionwaveform.h"

SoundSelectionDialog::SoundSelectionDialog(Mode mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SoundSelectionDialog),
    _mode (mode),
    _decoder(NULL),
    _musicSet (false)
{
    ui->setupUi(this);
    if (_mode == Mode::BACKGROUND_MUSIC) {
        _waveform = new MultiRegionWaveform (this);
    } else {
        _waveform = new VariableSelectionWaveform (this);
    }
    ui->upperHLayout->insertWidget(0, _waveform);
    ui->dummyWaveform->hide(); // This is a placeholder for UI development
    ui->playPauseButton->setText("");
    ui->rewindButton->setText("");
    ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
    ui->rewindButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaSkipBackward));

    connect (this, &SoundSelectionDialog::finished, this, &SoundSelectionDialog::dialogClosed);

    _player = new QMediaPlayer (this);
    _player->setNotifyInterval(75);
    connect(_player, &QMediaPlayer::positionChanged, this, &SoundSelectionDialog::playerPositionChanged);
    connect(_player, &QMediaPlayer::stateChanged, this, &SoundSelectionDialog::playerStateChanged);
    connect(_waveform, &Waveform::playheadManuallyChanged, this, &SoundSelectionDialog::setPlayhead);
}

SoundSelectionDialog::~SoundSelectionDialog()
{
    delete ui;
}

void SoundSelectionDialog::on_chooseMusicFileButton_clicked()
{
    // Stop any playing that is happening now...
    ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
    _player->pause();

    QString startingDirectory = "";
    switch (_mode) {
    case Mode::BACKGROUND_MUSIC:
        startingDirectory = "music";
        break;
    case Mode::SOUND_EFFECT:
        startingDirectory = "sfx";
        break;
    }

    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open Sound File"),
        startingDirectory,
        tr("Sound files (*.mp3;*.wav);;All files (*.*)"));
    loadFile(filename);
}

void SoundSelectionDialog::loadFile (const QString &filename)
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
        QObject::connect (_decoder, &QAudioDecoder::bufferReady, this, &SoundSelectionDialog::readBuffer);
        QObject::connect (_decoder, &QAudioDecoder::finished, this, &SoundSelectionDialog::readFinished);
    }
    _decoder->setAudioFormat(desiredFormat);
    _decoder->setSourceFilename(filename);
    _waveform->reset();
    _decoder->start();

    ui->removeMusicButton->setDisabled(false);
    ui->playPauseButton->setDisabled(false);
    ui->rewindButton->setDisabled(false);
}


void SoundSelectionDialog::readBuffer ()
{
    QAudioBuffer buffer = _decoder->read();
    _waveform->setDuration(_decoder->duration());
    _waveform->addBuffer(buffer);
}

void SoundSelectionDialog::readFinished ()
{
    Settings settings;
    _waveform->setSelectionStart(0);
    _waveform->setSelectionLength(0);
    _waveform->setDuration(_decoder->duration());

    if (_mode==Mode::BACKGROUND_MUSIC) {
        double preTitleDuration = settings.Get("settings/preTitleScreenDuration").toDouble();
        double titleDuration = settings.Get("settings/titleScreenDuration").toDouble();
        double creditsDuration = settings.Get("settings/creditsDuration").toDouble();
        double movieDuration = _movieDuration;

        MultiRegionWaveform *w = dynamic_cast<MultiRegionWaveform *> (_waveform);
        w->ClearRegions();
        qint64 offset = 0;
        w->AddRegion("Intro", offset, offset+preTitleDuration*1000, Qt::black);
        offset += preTitleDuration*1000;
        w->AddRegion("Title", offset, offset+titleDuration*1000, Qt::black);
        offset += titleDuration*1000;
        w->AddRegion("Movie", offset, offset+movieDuration*1000, Qt::blue);
        offset += movieDuration*1000;
        w->AddRegion("Credits", offset, offset+creditsDuration*1000, Qt::black);
    }

    _decoder->disconnect();
    _musicSet = true;
}

void SoundSelectionDialog::setMovieDuration (double duration)
{
    _movieDuration = duration;
    if (_musicSet && _mode==Mode::BACKGROUND_MUSIC) {
        // Trigger a re-creation of the regions
        readFinished ();
    }
}


void SoundSelectionDialog::on_playPauseButton_clicked()
{
    if (_player->state() == QMediaPlayer::StoppedState ||
        _player->state() == QMediaPlayer::PausedState) {
        _player->setPosition(_waveform->getPlayheadPosition());
        on_volumeSlider_valueChanged (ui->volumeSlider->value());
        _player->play();
    } else {
        _player->pause();
    }
}


void SoundSelectionDialog::playerPositionChanged (qint64 newPosition)
{
    if (newPosition != 0 &&
        newPosition == _player->duration()) {
        qDebug() << "Looping...";
        _player->setPosition(0);
        _player->play();
    }
    _waveform->setPlayheadPosition(newPosition);
}

void SoundSelectionDialog::playerStateChanged (QMediaPlayer::State state)
{
    switch (state) {
    case QMediaPlayer::StoppedState:
        ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
        break;
    case QMediaPlayer::PausedState:
        ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
        break;
    case QMediaPlayer::PlayingState:
        ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPause));
        break;
    }
}

void SoundSelectionDialog::setPlayhead (qint64 newPosition)
{
    if (_player->state() == QMediaPlayer::PlayingState) {
        _player->setPosition(newPosition);
    }
}

void SoundSelectionDialog::showEvent(QShowEvent *)
{
    if (!_musicSet) {
        on_chooseMusicFileButton_clicked();
    }
}

void SoundSelectionDialog::dialogClosed(int)
{
    _player->pause();
}

void SoundSelectionDialog::on_removeMusicButton_clicked()
{
    ui->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
    _player->pause();
    ui->removeMusicButton->setDisabled(true);
    ui->playPauseButton->setDisabled(true);
    ui->rewindButton->setDisabled(true);
    _waveform->reset();
    _musicSet = false;
}

void SoundSelectionDialog::on_rewindButton_clicked()
{
    _player->setPosition(0);
    _waveform->setPlayheadPosition(0);
}

SoundEffect SoundSelectionDialog::getSelectedSound () const
{
    if (_musicSet) {
        double in = (double)_waveform->getSelectionStart() / 1000.0;
        double out = in + (double)_waveform->getSelectionLength() / 1000.0;
        double linearVolume = QAudio::convertVolume(ui->volumeSlider->value() / qreal(200.0),
                                                      QAudio::LogarithmicVolumeScale,
                                                      QAudio::LinearVolumeScale);
        return SoundEffect (_filename, 0, in, out, linearVolume);
    } else {
        return SoundEffect ();
    }
}

void SoundSelectionDialog::on_volumeSlider_valueChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(200.0),
                                                  QAudio::LogarithmicVolumeScale,
                                                  QAudio::LinearVolumeScale);
    _player->setVolume (qRound(linearVolume * 100));
}

void SoundSelectionDialog::on_resetSelectionButton_clicked()
{
    // Trigger a recreation of the boxes
    readFinished();
}