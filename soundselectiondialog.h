#ifndef BACKGROUNDMUSICDIALOG_H
#define BACKGROUNDMUSICDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QString>
#include <QAudioDecoder>
#include <QGraphicsScene>
#include <QMediaPlayer>

#include "soundeffect.h"
#include "waveform.h"

namespace Ui {
class SoundSelectionDialog;
}

class SoundSelectionDialog : public QDialog
{
    Q_OBJECT

public:

    enum class Mode {
        BACKGROUND_MUSIC,
        SOUND_EFFECT
    };

    explicit SoundSelectionDialog(Mode mode, QWidget *parent = 0);
    ~SoundSelectionDialog();

    void setMovieDuration (double duration);

    void loadFile (const QString &filename);

    SoundEffect getSelectedSound () const;

    void setSound (const SoundEffect &sfx);

protected:
    virtual void showEvent(QShowEvent * event);

    void stressTest ();

private slots:
    void on_chooseMusicFileButton_clicked();
    void fileDialogAccepted();
    void fileDialogRejected();
    void on_playPauseButton_clicked();
    void readBuffer ();
    void readFinished ();
    void playerPositionChanged (qint64 newPosition);
    void playerStateChanged (QMediaPlayer::State state);
    void setPlayhead (qint64 newPosition);
    void dialogClosed(int);

    void on_removeMusicButton_clicked();

    void on_rewindButton_clicked();

    void on_volumeSlider_valueChanged(int value);

    void on_resetSelectionButton_clicked();

    void stressTestLoad ();

private:
    Ui::SoundSelectionDialog *ui;
    QFileDialog *_fileDialog;
    Mode _mode;
    QString _filename;
    QAudioDecoder *_decoder;
    QMediaPlayer *_player;
    Waveform *_waveform;
    SoundEffect _sfx;
    double _movieDuration;
    bool _musicSet;
    bool _loading;
};

#endif // BACKGROUNDMUSICDIALOG_H
