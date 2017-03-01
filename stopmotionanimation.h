#ifndef STOPMOTIONANIMATION_H
#define STOPMOTIONANIMATION_H

#include <QDialog>
#include <QErrorMessage>
#include <QtMultimedia/QCamera>
#include "movie.h"
#include "soundeffect.h"
#include "helpdialog.h"
#include "settingsdialog.h"
#include "backgroundmusicdialog.h"
#include "savefinalmoviedialog.h"

namespace Ui {
class StopMotionAnimation;
}

class StopMotionAnimation : public QDialog
{
    Q_OBJECT

public:
    explicit StopMotionAnimation(QWidget *parent = 0);
    ~StopMotionAnimation();

private slots:
    void on_startNewMovieButton_clicked();

    void on_addToPreviousButton_clicked();

    void on_createFinalMovieButton_clicked();

    void on_importButton_clicked();

    void on_settingButton_clicked();

    void on_helpButton_clicked();

    void on_takePhotoButton_clicked();

    void on_deletePhotoButton_clicked();

    void on_backgroundMusicButton_clicked();

    void on_soundEffectButton_clicked();

    void on_playButton_clicked();

    void on_horizontalSlider_sliderMoved(int value);

    void movieFrameChanged (unsigned int newFrame);

    void saveFinalMovieAccepted();

    void setBackgroundMusic();

private:
    Ui::StopMotionAnimation *ui;

protected:
    enum class State {LIVE, PLAYBACK, STILL};
    void setState (State newState);

private:
    QCamera *_camera;
    QCameraViewfinderSettings _viewFinderSettings;
    QErrorMessage _errorDialog;
    std::unique_ptr<Movie> _movie;
    State _state;

    HelpDialog _help;
    SettingsDialog _settings;
    BackgroundMusicDialog _backgroundMusic;
    SaveFinalMovieDialog _saveFinalMovie;


};

#endif // STOPMOTIONANIMATION_H
