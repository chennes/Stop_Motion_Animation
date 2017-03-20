#include "stopmotionanimation.h"
#include "ui_stopmotionanimation.h"

#include <QDateTime>
#include <QtMultimedia/QCameraInfo>
#include <QKeyEvent>
#include <QFileDialog>
#include "settings.h"


StopMotionAnimation::StopMotionAnimation(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::StopMotionAnimation),
    _camera(NULL)
{
    ui->setupUi(this);
    this->setWindowTitle(QCoreApplication::applicationName());
    connect (&this->_saveFinalMovie, &SaveFinalMovieDialog::accepted, this, &StopMotionAnimation::saveFinalMovieAccepted);
    Settings settings;
    int w = settings.Get("settings/imageWidth").toInt();
    int h = settings.Get("settings/imageHeight").toInt();
    QSize resolution (w,h);
    _viewFinderSettings.setResolution(resolution);
    on_startNewMovieButton_clicked();
    adjustSize();
    _overlayEffect = new PreviousFrameOverlayEffect();
    ui->cameraViewfinder->setGraphicsEffect(_overlayEffect);
    _overlayEffect->setEnabled(false);

    // Grab the x and z keys from everything that might conceivably get them:
    ui->addToPreviousButton->installEventFilter(this);
    ui->backgroundMusicButton->installEventFilter(this);
    ui->cameraViewfinder->installEventFilter(this);
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
}

StopMotionAnimation::~StopMotionAnimation()
{
    if (_movie) {
        _movie->save();
    }
    if (_camera) {
        delete _camera;
    }
    delete _overlayEffect;
    delete ui;
}

void StopMotionAnimation::on_startNewMovieButton_clicked()
{
    // Generate a new timestamp
    QDateTime local(QDateTime::currentDateTime());
    Settings settings;
    QString imageFilenameFormat = settings.Get("settings/imageFilenameFormat").toString();
    QString timestamp = local.toString (imageFilenameFormat);
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
    _movie = std::unique_ptr<Movie> (new Movie (timestamp));

    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    QString requestedCamera = settings.Get("settings/camera").toString();

    if (!cameras.empty()) {

        _camera = NULL;
        for (auto camera = cameras.begin(); camera!= cameras.end(); ++camera) {
            if (camera->description() == requestedCamera) {
                _camera = new QCamera(*camera);
                break;
            }
        }
        if (!_camera) {
            _camera = new QCamera(cameras.back());
        }
        _camera->setViewfinder (ui->cameraViewfinder);
        _camera->setViewfinderSettings(_viewFinderSettings);
        _camera->setCaptureMode(QCamera::CaptureStillImage);
        _camera->start();

        connect (_movie.get(), &Movie::frameChanged,
                 this, &StopMotionAnimation::movieFrameChanged);

        setState (State::LIVE);

        ui->takePhotoButton->setDisabled(false);
        ui->takePhotoButton->setDefault(true);

    } else {
        // This should display an error of some kind...
        ui->videoLabel->setText("<big><b>ERROR:</b> No camera found. Plug in a camera and press the <kbd>Save and Start New Movie</kbd> button.</big>");
        ui->videoLabel->show();
        ui->cameraViewfinder->hide();
        ui->takePhotoButton->setDisabled(true);
    }

    updateInterfaceForNewFrame();
    adjustSize();
}

void StopMotionAnimation::on_addToPreviousButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Movie File"), "Image Files", tr("Stop-motion movie files (*.json);;All files (*.*)"));

    if (fileName.length() > 0) {
        bool success = _movie->load(fileName);
        if (!success) {
            _errorDialog.showMessage("Could not load the movie file " + fileName);
        } else {
            updateInterfaceForNewFrame();
            setState(State::LIVE);
        }
    }
}

void StopMotionAnimation::on_createFinalMovieButton_clicked()
{
    _saveFinalMovie.reset(_movie->getName());
    _saveFinalMovie.show();
}

void StopMotionAnimation::saveFinalMovieAccepted()
{
    QString filename = _saveFinalMovie.filename();
    QString title = _saveFinalMovie.movieTitle();
    QString credits = _saveFinalMovie.credits();
    try {
        _movie->encodeToFile (filename, title, credits);
    } catch (const Movie::EncodingFailedException &e) {
        _errorDialog.showMessage(e.message());
    }
}

void StopMotionAnimation::on_importButton_clicked()
{
    QString name = QFileDialog::getExistingDirectory();
    Settings settings;
    QString imageFileType = settings.Get("settings/imageFileType").toString().toLower();
    QDir directory (name, "*."+imageFileType);
    QStringList fileList = directory.entryList (QDir::NoFilter, QDir::Name);
    foreach (const QString &filename, fileList) {
        try {
            _movie->importFrame(name + "/" + filename);
            updateInterfaceForNewFrame();
        } catch (Movie::ImportFailedException &) {
            _errorDialog.showMessage("Failed to import the file " + filename);
            return;
        }
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
    if (_state == State::LIVE) {

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
    _backgroundMusic.show();
    // Connect its slot to set the music
    connect (&_backgroundMusic, &BackgroundMusicDialog::accepted, this, &StopMotionAnimation::setBackgroundMusic);
}

void StopMotionAnimation::setBackgroundMusic ()
{
    _movie->addBackgroundMusic (_backgroundMusic.getBackgroundMusic());
}

void StopMotionAnimation::on_soundEffectButton_clicked()
{
    // Launch the sound effects dialog
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

void StopMotionAnimation::on_horizontalSlider_sliderMoved(int value)
{
    if (value > (int)_movie->getNumberOfFrames()) {
        setState (State::LIVE);
    } else {
        setState (State::STILL);
        _movie->setStillFrame (value-1, ui->videoLabel);
    }
}

void StopMotionAnimation::movieFrameChanged (unsigned int newFrame)
{
    switch (_state) {
    case State::LIVE:
        // If the movie frame changed while we were live, just ignore it
        break;
    case State::PLAYBACK:
        // Fall through...
    case State::STILL:
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
        ui->videoLabel->hide();
        ui->cameraViewfinder->show();
        ui->frameNumberLabel->setText("(Live)");
        ui->playButton->setText("Play");
        ui->horizontalSlider->setValue(_movie->getNumberOfFrames()+1);
        ui->takePhotoButton->setDefault(true);
        break;
    case State::PLAYBACK:
        ui->playButton->setText("Stop");
        ui->cameraViewfinder->hide();
        ui->videoLabel->show();
        ui->playButton->setDefault(true);
        break;
    case State::STILL:
        ui->playButton->setText("Play");
        ui->cameraViewfinder->hide();
        ui->videoLabel->show();
        ui->playButton->setDefault(true);
        break;
    }
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
        ui->soundEffectButton->setDisabled(false);
        ui->backgroundMusicButton->setDisabled(false);
        ui->createFinalMovieButton->setDisabled(false);
    } else {
        ui->playButton->setDisabled(true);
        ui->horizontalSlider->setDisabled(true);
        ui->deletePhotoButton->setDisabled(true);
        ui->soundEffectButton->setDisabled(true);
        ui->backgroundMusicButton->setDisabled(true);
        ui->createFinalMovieButton->setDisabled(true);
    }
}



void StopMotionAnimation::keyPressEvent(QKeyEvent * e)
{
    if (e->key() == 'x') {
        if (_state == State::LIVE && _keydownState == KeydownState::NONE) {
            _keydownState = KeydownState::OVERLAY_FRAME;
        }
    } else if (e->key() == 'z') {
        if (_state == State::LIVE && _keydownState == KeydownState::NONE) {
            _keydownState = KeydownState::PREVIOUS_FRAME;
        }
    }
    if (_keydownState == KeydownState::OVERLAY_FRAME) {
        _overlayEffect->setEnabled(true);
    } else {
        _overlayEffect->setEnabled(false);
    }
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
    if (e->key() == 'x') {
        if (_keydownState == KeydownState::OVERLAY_FRAME) {
            _keydownState = KeydownState::NONE;
        }
    } else if (e->key() == 'z') {
        if (_keydownState == KeydownState::PREVIOUS_FRAME) {
            _keydownState = KeydownState::NONE;
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
    QMainWindow::keyReleaseEvent(e);
}

