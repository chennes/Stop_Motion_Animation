#include "stopmotionanimation.h"
#include "ui_stopmotionanimation.h"

#include <QDateTime>
#include <QtMultimedia/QCameraInfo>
#include <QKeyEvent>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include "settings.h"
#include "version.h"
#include "importprogressdialog.h"

#include <memory>


StopMotionAnimation::StopMotionAnimation(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::StopMotionAnimation),
    _camera(NULL),
    _viewfinder(NULL),
    _cameraMonitor (NULL),
    _keydownState (KeydownState::NONE),
    _backgroundMusic (SoundSelectionDialog::Mode::BACKGROUND_MUSIC, this),
    _soundEffects (SoundSelectionDialog::Mode::SOUND_EFFECT, this)
{
    ui->setupUi(this);

    _viewfinder = new QCameraViewfinder(this);
    _viewfinder->setBaseSize(640, 480);
    _viewfinder->setMinimumSize(640,480);
    _viewfinder->setMaximumSize(640,480);
    ui->videoRegionLayout->insertWidget(2,_viewfinder); // After the spacer and still image widgets
    _viewfinder->hide();

    this->setWindowTitle(QCoreApplication::applicationName() + " -- v" + APP_VERSION + "-" + APP_REVISION);
    connect (&this->_saveFinalMovie, &SaveFinalMovieDialog::accepted, this, &StopMotionAnimation::saveFinalMovieAccepted);
    Settings settings;
    int w = settings.Get("settings/imageWidth").toInt();
    int h = settings.Get("settings/imageHeight").toInt();
    QSize resolution (w,h);
    _viewFinderSettings.setResolution(resolution);
    startNewMovie();
    adjustSize();
    _overlayEffect = new PreviousFrameOverlayEffect();
    _viewfinder->setGraphicsEffect(_overlayEffect);
    _overlayEffect->setEnabled(false);

    // Grab the x, z, and spacebar keys from everything that might conceivably get them:
    ui->addToPreviousButton->installEventFilter(this);
    ui->backgroundMusicButton->installEventFilter(this);
    _viewfinder->installEventFilter(this);
    ui->createFinalMovieButton->installEventFilter(this);
    ui->deletePhotoButton->installEventFilter(this);
    ui->helpButton->installEventFilter(this);
    ui->horizontalSlider->installEventFilter(this);
    ui->importButton->installEventFilter(this);
    ui->playButton->installEventFilter(this);
    ui->settingButton->installEventFilter(this);
    ui->soundEffectButton->installEventFilter(this);
    ui->startNewMovieButton->installEventFilter(this);
    ui->takePhotoButton->installEventFilter(this);
    ui->soundEffectNumberLabel->installEventFilter(this);

    connect (&_backgroundMusic, &SoundSelectionDialog::accepted, this, &StopMotionAnimation::setBackgroundMusic);
    connect (&_soundEffects, &SoundSelectionDialog::accepted, this, &StopMotionAnimation::setSoundEffect);
    connect (&_addToPrevious, &AddToPreviousMovieDialog::accepted, this, &StopMotionAnimation::addToPrevious);

    // The SFX list dialog works a bit like a "remote control" for the main interface, all its main functions are really implemented here
    connect (&_sfxListDialog, &SoundEffectListDialog::selected, this, &StopMotionAnimation::soundEffectListSelected);
    connect (&_sfxListDialog, &SoundEffectListDialog::edit, this, &StopMotionAnimation::soundEffectListEdit);
    connect (&_sfxListDialog, &SoundEffectListDialog::remove, this, &StopMotionAnimation::soundEffectListRemove);
    connect (&_sfxListDialog, &SoundEffectListDialog::play, this, &StopMotionAnimation::soundEffectListPlay);

    // Remove the Help icon menu from the Help dialog
    Qt::WindowFlags flags = _help.windowFlags();
    Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
    flags = flags & (~helpFlag);
    _help.setWindowFlags(flags);
}

StopMotionAnimation::~StopMotionAnimation()
{
    if (_movie) {
        _movie->save();
    }
    if (_camera) {
        delete _camera;
    }
    if (_cameraMonitor) {
        _cameraMonitor->requestInterruption();
        _cameraMonitor->wait(1000);
    }
    delete _overlayEffect;
    delete ui;
}

void StopMotionAnimation::on_startNewMovieButton_clicked()
{
    std::unique_ptr<QMessageBox> message (new QMessageBox(QMessageBox::Information, "Loading", "Connecting to your camera, just a moment...", QMessageBox::Ok));
    message->show();
    QApplication::processEvents();
    startNewMovie ();
}

void StopMotionAnimation::startNewMovie ()
{
    // Generate a new timestamp
    QDateTime local(QDateTime::currentDateTime());
    Settings settings;
    auto imageFilenameFormat = settings.Get("settings/imageFilenameFormat").toString();
    auto timestamp = local.toString (imageFilenameFormat);
    if (timestamp.length() == 0) {
        timestamp = local.toString ("yyyy-MM-dd-hh-mm-ss");
    }
    // Make sure it's a valid-ish filename by removing the most common invalid characters
    timestamp.replace('/',"-");
    timestamp.replace('\\',"-");
    timestamp.replace('*',"-");
    timestamp.replace(':',"-");

    // Update the label
    ui->movieNameLabel->setText(timestamp);

    if (_movie) {
        _movie->save();
    }
    if (_camera) {
        delete _camera;
        _camera = NULL;
    }
    if (_cameraMonitor) {
        _cameraMonitor->requestInterruption();
        _cameraMonitor->wait(1000);
        delete _cameraMonitor;
        _cameraMonitor = NULL;
    }
    _movie = std::unique_ptr<Movie> (new Movie (timestamp));
    connect (_movie.get(), &Movie::frameChanged,
             this, &StopMotionAnimation::movieFrameChanged);

    auto cameras = QCameraInfo::availableCameras();
    auto requestedCamera = settings.Get("settings/camera").toString();

    if (!cameras.empty()) {

        _camera = NULL;
        for (auto camera: cameras) {
            if (camera.description() == requestedCamera) {
                _camera = new QCamera(camera);
                _cameraInfo = camera;
                break;
            }
        }
        if (!_camera) {
            _camera = new QCamera(cameras.back());
            _cameraInfo = cameras.back();
        }
        _cameraMonitor = new CameraMonitor (this, _cameraInfo);
        connect (_cameraMonitor, &CameraMonitor::cameraLost, this, &StopMotionAnimation::cameraLost);
        connect (_cameraMonitor, &CameraMonitor::finished, _cameraMonitor, &QObject::deleteLater);
        _camera->setViewfinder (_viewfinder);
        _camera->setViewfinderSettings(_viewFinderSettings);
        _camera->setCaptureMode(QCamera::CaptureStillImage);
        _camera->start();
        _cameraMonitor->start();
    } else {
        // This should display an error of some kind...
        ui->videoLabel->setText("<big><b>ERROR:</b> No camera found. Plug in a camera and press the <kbd>Save and Start a New Movie</kbd> button.</big>");
        _viewfinder->hide();
        ui->videoLabel->show();
    }

    setState (State::LIVE);
    updateInterfaceForNewFrame();
    adjustSize();
    updateSoundEffectLabel();
}

void StopMotionAnimation::cameraLost ()
{
    if (_camera) {
        delete _camera;
        _camera = NULL;
        ui->videoLabel->setText("<big><b>ERROR:</b> Camera disconnected. Plug in a camera and press the <kbd>Save and Start a New Movie</kbd> button.</big>");
        setState (State::LIVE);
        updateInterfaceForNewFrame();
    }
    _cameraMonitor->requestInterruption();
    _cameraMonitor->wait(1000);
    delete _cameraMonitor;
    _cameraMonitor = NULL;
}

void StopMotionAnimation::on_addToPreviousButton_clicked()
{
    _addToPrevious.show();
}

void StopMotionAnimation::addToPrevious ()
{
    QString fileName = _addToPrevious.getSelectedMovie();
    if (fileName.length() > 0) {
        // See if we can read it first:
        QFileInfo f (fileName);
        QString filenameNoPath = f.fileName();
        if (!f.isReadable()) {
            _errorDialog.showMessage("Cannot read the file " + filenameNoPath);
        }

        bool success = _movie->load(fileName);
        if (!success) {
            _errorDialog.showMessage("Could not load the file " + filenameNoPath + " as a PLS Stop Motion Creator movie file");
        } else {
            ui->movieNameLabel->setText(_movie->getName());
            updateInterfaceForNewFrame();
            setState(State::STILL); // We MUST set the state before the frame
            _movie->setStillFrame (0, ui->videoLabel);
            updateSoundEffectLabel();
        }
        try {
            _overlayEffect->setPreviousFrame(_movie->getMostRecentFrame());
        } catch (PreviousFrameOverlayEffect::LoadFailedException &e) {
            //_errorDialog.showMessage("Failed to load the previous frame into the overlay layer.");
            _errorDialog.showMessage(e.message());
        }
    }
}

void StopMotionAnimation::on_createFinalMovieButton_clicked()
{
    _saveFinalMovie.reset(_movie->getEncodingFilename(), _movie->getEncodingTitle(), _movie->getEncodingCredits());
    _saveFinalMovie.show();
}

void StopMotionAnimation::saveFinalMovieAccepted()
{
    QString filename = _saveFinalMovie.filename();
    QString title = _saveFinalMovie.movieTitle();
    QString credits = _saveFinalMovie.credits();
    try {
        std::unique_ptr<QMessageBox> message (new QMessageBox(QMessageBox::Information, "Encoding", "Creating your movie, just a moment...", QMessageBox::NoButton));
        message->show();
        QApplication::processEvents();
        _movie->encodeToFile (filename, title, credits);
        QDesktopServices::openUrl (QUrl::fromLocalFile(filename));
    } catch (const Movie::EncodingFailedException &e) {
        _errorDialog.showMessage(e.message());
    } catch (...) {
        _errorDialog.showMessage("An unknown error occurred while encoding.");
    }
}

void StopMotionAnimation::on_importButton_clicked()
{
    Settings settings;
    QString imageFileType = settings.Get("settings/imageFileType").toString().toLower();
    QString imageFileString = "Images (*." + imageFileType + ");; All files (*.*)";
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select image files to import"), "Image Files", imageFileString);
    std::unique_ptr<ImportProgressDialog> progressDialog (new ImportProgressDialog(this));
    progressDialog->setNumberOfFramesToImport(files.size());
    progressDialog->show();
    QApplication::processEvents();
    for (auto filename: files) {
        try {
            _movie->importFrame(filename);
            updateInterfaceForNewFrame();
            progressDialog->increment();
            QApplication::processEvents();
        } catch (Movie::ImportFailedException &) {
            _errorDialog.showMessage("Failed to import the file " + filename);
            return;
        }
    }
    try {
        _overlayEffect->setPreviousFrame(_movie->getMostRecentFrame());
    } catch (PreviousFrameOverlayEffect::LoadFailedException &e) {
        //_errorDialog.showMessage("Failed to load the previous frame into the overlay layer.");
        _errorDialog.showMessage(e.message());
    }
    updateInterfaceForNewFrame();
}

void StopMotionAnimation::on_settingButton_clicked()
{
    // Launch the settings dialog
    _settings.load();
    auto result = _settings.exec();
    if (result == QDialog::Accepted) {
        _settings.store();
    }
}

void StopMotionAnimation::on_helpButton_clicked()
{
    _help.show();
}

void StopMotionAnimation::on_takePhotoButton_clicked()
{
    // Store the frame
    if (_state == State::LIVE && _camera) {

        // Get the frame out of the camera:
        _movie->addFrame (_camera);

        // Update the overlay effect:
        try {
            _overlayEffect->setPreviousFrame(_movie->getMostRecentFrame());
        } catch (PreviousFrameOverlayEffect::LoadFailedException &e) {
            //_errorDialog.showMessage("Failed to load the previous frame into the overlay layer.");
            _errorDialog.showMessage(e.message());
        }

        updateInterfaceForNewFrame();
    }
}

void StopMotionAnimation::on_deletePhotoButton_clicked()
{
    _movie->deleteLastFrame();
   updateInterfaceForNewFrame();
}

void StopMotionAnimation::on_backgroundMusicButton_clicked()
{
    Settings settings;
    qint32 framesPerSecond = settings.Get("settings/framesPerSecond").toInt();
    _backgroundMusic.setMovieDuration((double)_movie->getNumberOfFrames() / (double)framesPerSecond);
    _backgroundMusic.setSound (_movie->getBackgroundMusic());
    _backgroundMusic.show();
}

void StopMotionAnimation::setBackgroundMusic ()
{
    _movie->addBackgroundMusic (_backgroundMusic.getSelectedSound());
}

void StopMotionAnimation::setSoundEffect()
{
    _movie->addSoundEffect (_soundEffects.getSelectedSound());
    ui->soundEffectButton->setText("Edit sound effect...");
    updateSoundEffectLabel();
    if (_sfxListDialog.isVisible()) {
        _sfxListDialog.RemoveAllSoundEffects();
        _sfxListDialog.AddSoundEffects(_movie->getSoundEffects());
        _sfxListDialog.activateWindow();
    }
}

void StopMotionAnimation::on_soundEffectButton_clicked()
{
    if (_state == State::STILL) {
        int frame = ui->frameNumberLabel->text().toInt() - 1;
        const SoundEffect &sfx (_movie->getSoundEffect(frame));
        if (sfx && _movie->getSoundEffects().length() >= MAX_SOUND_EFFECTS) {
            _errorDialog.showMessage("Sound effect limit reached; no more effects can be added.");
        } else {
            _soundEffects.setSound(sfx);
            _soundEffects.show();
        }
    }
}

void StopMotionAnimation::on_playButton_clicked()
{
    if (_state != State::PLAYBACK) {
        setState (State::PLAYBACK);
        _movie->play (0, ui->videoLabel);
    } else {
        setState (State::LIVE);
        _movie->stop();
    }
}

void StopMotionAnimation::movieFrameSliderValueChanged(int value)
{
    if (value > (int)_movie->getNumberOfFrames()) {
        setState (State::LIVE);
    } else {
        if (_state != State::PLAYBACK) {
            setState (State::STILL);
            _movie->setStillFrame (value-1, ui->videoLabel);
        }
    }
}

void StopMotionAnimation::movieFrameChanged (unsigned int newFrame)
{
    SoundEffect sfx;
    switch (_state) {
    case State::LIVE:
        // If the movie frame changed while we were live, just ignore it
        break;
    case State::STILL:
        // Is there a sound effect for this frame?
        sfx = _movie->getSoundEffect (newFrame);
        if (sfx) {
            ui->soundEffectButton->setText("Edit sound effect...");
        } else {
            ui->soundEffectButton->setText("Add sound effect...");
        }
        // Fall through...

    case State::PLAYBACK:
        ui->frameNumberLabel->setText (QString::number(newFrame+1));
        ui->horizontalSlider->setValue(newFrame+1);

        break;
    }
}

void StopMotionAnimation::setState (State newState)
{
    _state = newState;
    switch (_state) {
    case State::LIVE:
        if (_camera) {
            ui->videoLabel->hide();
            _viewfinder->show();
            ui->takePhotoButton->setDefault(true);
            ui->takePhotoButton->setEnabled(true);
        } else {
            _viewfinder->hide();
            ui->videoLabel->show();
            ui->takePhotoButton->setDefault(false);
            ui->takePhotoButton->setEnabled(false);
        }
        ui->frameNumberLabel->setText("(Live)");
        ui->playButton->setText("Play");
        ui->horizontalSlider->setValue(_movie->getNumberOfFrames()+1);
        ui->soundEffectButton->setEnabled(false);
        break;
    case State::PLAYBACK:
        ui->frameNumberLabel->setText(QString::number(ui->horizontalSlider->value()));
        ui->playButton->setText("Stop");
        _viewfinder->hide();
        ui->videoLabel->show();
        ui->playButton->setDefault(true);
        ui->takePhotoButton->setEnabled(false);
        ui->soundEffectButton->setEnabled(false);
        break;
    case State::STILL:
        ui->frameNumberLabel->setText(QString::number(ui->horizontalSlider->value()));
        ui->playButton->setText("Play");
        _viewfinder->hide();
        ui->videoLabel->show();
        ui->playButton->setDefault(true);
        ui->takePhotoButton->setEnabled(false);
        ui->soundEffectButton->setEnabled(true);
        break;
    }
    qDebug() << "Video label: " << ui->videoLabel->isVisible();
    qDebug() << "Viewfinder:  " << _viewfinder->isVisible();
}



void StopMotionAnimation::updateInterfaceForNewFrame()
{
    int numberOfFrames = _movie->getNumberOfFrames();
    ui->numberOfFramesLabel->setText (QString::number(numberOfFrames));
    ui->horizontalSlider->setRange(1,numberOfFrames+1); // The +1 is because the very last "frame" is the live view
    ui->horizontalSlider->setSliderPosition(numberOfFrames+1);

    Settings settings;
    qint32 framesPerSecond = settings.Get("settings/framesPerSecond").toInt();
    double movieLength = (double)numberOfFrames / (double)framesPerSecond;
    if (movieLength < 60) {
        ui->movieLengthLabel->setText(QTime(0,0,0,0).addMSecs(movieLength*1000).toString("s.zzz") + " seconds");
    } else {
        ui->movieLengthLabel->setText(QTime(0,0,0,0).addMSecs(movieLength*1000).toString("m:ss.zzz"));
    }

    if (numberOfFrames > 0) {
        ui->playButton->setDisabled(false);
        ui->horizontalSlider->setDisabled(false);
        ui->deletePhotoButton->setDisabled(false);
        ui->backgroundMusicButton->setDisabled(false);
        ui->createFinalMovieButton->setDisabled(false);
    } else {
        ui->playButton->setDisabled(true);
        ui->horizontalSlider->setDisabled(true);
        ui->deletePhotoButton->setDisabled(true);
        ui->backgroundMusicButton->setDisabled(true);
        ui->createFinalMovieButton->setDisabled(true);

        // If we just deleted the last frame (or something), we have to make sure we
        // are stopped, because with the play button disabled there is no stop button!
        setState (State::LIVE);
        _movie->stop();
    }
}



void StopMotionAnimation::keyPressEvent(QKeyEvent * e)
{
    QMainWindow::keyPressEvent(e);
}

bool StopMotionAnimation::eventFilter(QObject *, QEvent *event)
{
    bool handled = false;
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_X) {
            // Special X handling
            if (_state == State::LIVE && _keydownState == KeydownState::NONE) {
                _keydownState = KeydownState::OVERLAY_FRAME;
            }
            handled = true;
        } else if (keyEvent->key() == Qt::Key_Z) {
            // Special Z handling
            if (_state == State::LIVE && _keydownState == KeydownState::NONE) {
                _keydownState = KeydownState::PREVIOUS_FRAME;
            }
            handled = true;
        } else if (keyEvent->key() == Qt::Key_Space) {
            if (_state == State::LIVE && _camera) {
                ui->takePhotoButton->click();
            } else {
                ui->playButton->click();
            }
            handled = true;
        } else {
            handled = false;
        }
    } else if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_X) {
            // Special X handling
            if (_state == State::LIVE && _keydownState == KeydownState::OVERLAY_FRAME) {
                _keydownState = KeydownState::NONE;
            }
            handled = true;
        } else if (keyEvent->key() == Qt::Key_Z) {
            // Special Z handling
            if (_state == State::LIVE && _keydownState == KeydownState::PREVIOUS_FRAME) {
                _keydownState = KeydownState::NONE;
            }
            handled = true;
        } else if (keyEvent->key() == Qt::Key_Space) {
            // We actually handled it on the keydown...
            handled = true;
        } else {
            handled = false;
        }
    }
    if (_keydownState == KeydownState::OVERLAY_FRAME) {
        _overlayEffect->setEnabled(true);
        _overlayEffect->setMode(PreviousFrameOverlayEffect::Mode::BLEND);
    } else if (_keydownState == KeydownState::PREVIOUS_FRAME) {
        _overlayEffect->setEnabled(true);
        _overlayEffect->setMode(PreviousFrameOverlayEffect::Mode::PREVIOUS);
    } else {
        _overlayEffect->setEnabled(false);
    }
    return handled;
}


void StopMotionAnimation::keyReleaseEvent(QKeyEvent * e)
{
    QMainWindow::keyReleaseEvent(e);
}

void StopMotionAnimation::on_soundEffectNumberLabel_linkActivated(const QString &)
{
    _sfxListDialog.RemoveAllSoundEffects();
    _sfxListDialog.AddSoundEffects(_movie->getSoundEffects());
    _sfxListDialog.show();
}

void StopMotionAnimation::soundEffectListSelected(const SoundEffect &sfx)
{
    ui->horizontalSlider->setValue(sfx.getStartFrame()+1);
}

void StopMotionAnimation::soundEffectListEdit(const SoundEffect &)
{
    on_soundEffectButton_clicked();
}

void StopMotionAnimation::soundEffectListRemove(const SoundEffect &sfx)
{
    _movie->removeSoundEffect(sfx);
    updateSoundEffectLabel();
}

void StopMotionAnimation::soundEffectListPlay(const SoundEffect &sfx)
{
    _movie->playSoundEffect (sfx, ui->videoLabel);
}

void StopMotionAnimation::updateSoundEffectLabel()
{
    QString newLabelText;
    QString numSFX = QString::number(_movie->getSoundEffects().length());
    newLabelText = QString("<html><body><p align=\"center\"><a href=\"#\">") + numSFX + QString (" of ") + QString::number(MAX_SOUND_EFFECTS) + QString(" sounds</a></p></body></html>");
    ui->soundEffectNumberLabel->setText(newLabelText);
}
