#ifndef BACKGROUNDMUSICDIALOG_H
#define BACKGROUNDMUSICDIALOG_H

#include <QDialog>
#include <QString>
#include <QAudioDecoder>
#include <QGraphicsScene>
#include <QMediaPlayer>

#include "soundeffect.h"

namespace Ui {
class BackgroundMusicDialog;
}

class BackgroundMusicDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BackgroundMusicDialog(QWidget *parent = 0);
    ~BackgroundMusicDialog();

    void setMovieDuration (double duration);

    void loadFile (const QString &filename);

    SoundEffect getBackgroundMusic () const;

protected:
    virtual void showEvent(QShowEvent * event);


private slots:
    void on_chooseMusicFileButton_clicked();
    void on_playPauseButton_clicked();
    void readBuffer ();
    void playerPositionChanged (qint64 newPosition);
    void playerStateChanged (QMediaPlayer::State state);
    void setPlayhead (qint64 newPosition);
    void dialogClosed(int);

    void on_removeMusicButton_clicked();

    void on_rewindButton_clicked();

    void on_volumeSlider_valueChanged(int value);

private:
    Ui::BackgroundMusicDialog *ui;
    QString _filename;
    QAudioDecoder *_decoder;
    QGraphicsScene *_scene;
    QMediaPlayer *_player;
    double _movieDuration;
    bool _firstLaunch;
};

#endif // BACKGROUNDMUSICDIALOG_H
