#ifndef STOPMOTIONANIMATION_H
#define STOPMOTIONANIMATION_H

#include <QMainWindow>
#include <QErrorMessage>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraInfo>
#include <QCameraViewfinder>
#include "movie.h"
#include "soundeffect.h"
#include "helpdialog.h"
#include "settingsdialog.h"
#include "soundselectiondialog.h"
#include "savefinalmoviedialog.h"
#include "addtopreviousmoviedialog.h"
#include "soundeffectlistdialog.h"
#include "previousframeoverlayeffect.h"
#include "cameramonitor.h"

namespace Ui {
class StopMotionAnimation;
}

class StopMotionAnimation : public QMainWindow
{
    Q_OBJECT

public:
    explicit StopMotionAnimation(QWidget *parent = 0);
    ~StopMotionAnimation();

    void startNewMovie ();

private slots:
    void on_startNewMovieButton_clicked();

    void on_addToPreviousButton_clicked();

    void cameraLost ();

    void addToPrevious ();

    void on_createFinalMovieButton_clicked();

    void on_importButton_clicked();

    void on_settingButton_clicked();

    void on_helpButton_clicked();

    void on_takePhotoButton_clicked();

    void on_deletePhotoButton_clicked();

    void on_backgroundMusicButton_clicked();

    void on_soundEffectButton_clicked();

    void on_playButton_clicked();

    void movieFrameSliderValueChanged(int value);

    void movieFrameChanged (unsigned int newFrame);

    void saveFinalMovieAccepted();

    void setBackgroundMusic();

    void setSoundEffect();

    void updateInterfaceForNewFrame();

    void on_soundEffectNumberLabel_linkActivated(const QString &link);

    void soundEffectListSelected(const SoundEffect &sfx);

    void soundEffectListEdit(const SoundEffect &sfx);

    void soundEffectListRemove(const SoundEffect &sfx);

    void soundEffectListPlay(const SoundEffect &sfx);

    void updateSoundEffectLabel ();

private:
    Ui::StopMotionAnimation *ui;

protected:
    enum class State {LIVE, PLAYBACK, STILL};
    enum class KeydownState {NONE, OVERLAY_FRAME, PREVIOUS_FRAME};
    void setState (State newState);

    virtual bool eventFilter (QObject *object, QEvent *event);

    virtual void keyPressEvent(QKeyEvent * e);
    virtual void keyReleaseEvent(QKeyEvent * e);


private:
    static const int MAX_SOUND_EFFECTS = 25;
    QCamera *_camera;
    QCameraViewfinder *_viewfinder;
    QCameraInfo _cameraInfo;
    CameraMonitor *_cameraMonitor;
    QCameraViewfinderSettings _viewFinderSettings;
    QErrorMessage _errorDialog;
    std::unique_ptr<Movie> _movie;
    State _state;
    KeydownState _keydownState;

    PreviousFrameOverlayEffect *_overlayEffect;

    HelpDialog _help;
    SettingsDialog _settings;
    SoundSelectionDialog _backgroundMusic;
    SoundSelectionDialog _soundEffects;
    SaveFinalMovieDialog _saveFinalMovie;
    AddToPreviousMovieDialog _addToPrevious;
    SoundEffectListDialog _sfxListDialog;

};

#endif // STOPMOTIONANIMATION_H
