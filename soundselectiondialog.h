#ifndef BACKGROUNDMUSICDIALOG_H
#define BACKGROUNDMUSICDIALOG_H

#include <QDialog>
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


protected:
    virtual void showEvent(QShowEvent * event);


private slots:
    void on_chooseMusicFileButton_clicked();
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

private:
    Ui::SoundSelectionDialog *ui;
    Mode _mode;
    QString _filename;
    QAudioDecoder *_decoder;
    QGraphicsScene *_scene;
    QMediaPlayer *_player;
    Waveform *_waveform;
    double _movieDuration;
    bool _musicSet;
};

#endif // BACKGROUNDMUSICDIALOG_H
