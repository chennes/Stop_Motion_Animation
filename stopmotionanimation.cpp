#include "stopmotionanimation.h"
#include "ui_stopmotionanimation.h"

#include <QDateTime>
#include <QtMultimedia/QCameraInfo>
#include <QKeyEvent>
#include <QFileDialog>
#include <QSettings>


StopMotionAnimation::StopMotionAnimation(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StopMotionAnimation),
    _camera(NULL)
{
    ui->setupUi(this);
    connect (&this->_saveFinalMovie, &SaveFinalMovieDialog::accepted, this, &StopMotionAnimation::saveFinalMovieAccepted);
    QSettings settings;
    QSize resolution = settings.value("settings/resolution",QSize(640,480)).toSize();
    _viewFinderSettings.setResolution(resolution);
    on_startNewMovieButton_clicked();
    adjustSize();
}

StopMotionAnimation::~StopMotionAnimation()
{
    if (_movie) {
        _movie->save();
    }
    if (_camera) {
        delete _camera;
    }
    delete ui;
}

void StopMotionAnimation::on_startNewMovieButton_clicked()
{
    // Generate a new timestamp
    QDateTime local(QDateTime::currentDateTime());
    QSettings settings;
    QString imageFilenameFormat = settings.value("settings/imageFilenameFormat","yyyy-MM-dd-hh-mm-ss").toString();
    QString timestamp = local.toString (imageFilenameFormat);
    if (timestamp.length() == 0) {
        timestamp = local.toString ("yyyy-MM-dd-hh-mm-ss");
    }
    // Make sure it's a valid-ish filename
    timestamp.replace('/',"-");
    timestamp.replace('\\',"-");

    // Update the label
    ui->movieNameLabel->setText(timestamp);

    // Set frame counting widgets to zero and disable everything but the "Take Photo" button
    ui->frameNumberLabel->setText ("0");
    ui->numberOfFramesLabel->setText ("0");

    ui->playButton->setDisabled(true);
    ui->horizontalSlider->setDisabled(true);
    ui->deletePhotoButton->setDisabled(true);
    ui->soundEffectButton->setDisabled(true);
    ui->backgroundMusicButton->setDisabled(true);
    ui->createFinalMovieButton->setDisabled(true);

    if (_movie) {
        _movie->save();
    }
    if (_camera) {
        delete _camera;
        _camera = NULL;
    }
    _movie = std::unique_ptr<Movie> (new Movie (timestamp));

    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    QString requestedCamera = settings.value("settings/camera","Always use default").toString();

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

    adjustSize();
}

void StopMotionAnimation::on_addToPreviousButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Movie File"), "Image Files", tr("Stop-motion movie files (*.json);;All files (*.*)"));
    bool success = _movie->load(fileName);
    if (!success) {
        _errorDialog.showMessage("Could not load the movie file " + fileName);
    } else {
        qint32 numberOfFrames = _movie->getNumberOfFrames();
        ui->movieNameLabel->setText(_movie->getName());
        ui->numberOfFramesLabel->setText (QString::number(numberOfFrames));
        ui->horizontalSlider->setMaximum(numberOfFrames+1);
        ui->horizontalSlider->setValue(numberOfFrames+1);
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
        setState(State::LIVE);
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
    QSettings settings;
    QString imageFileType = settings.value("settings/imageFileType","JPG").toString().toLower();
    QDir directory (name, "*."+imageFileType);
    QStringList fileList = directory.entryList (QDir::NoFilter, QDir::Name);
    foreach (const QString &filename, fileList) {
        try {
            _movie->importFrame(name + "/" + filename);
            qint32 numberOfFrames = _movie->getNumberOfFrames();
            ui->numberOfFramesLabel->setText (QString::number(numberOfFrames));
            ui->horizontalSlider->setMaximum(numberOfFrames+1);
            ui->horizontalSlider->setValue(numberOfFrames+1);
        } catch (Movie::ImportFailedException &e) {
            _errorDialog.showMessage("Failed to import the file " + filename);
            return;
        }
    }
    if (_movie->getNumberOfFrames() > 0) {
        ui->playButton->setDisabled(false);
        ui->horizontalSlider->setDisabled(false);
        ui->deletePhotoButton->setDisabled(false);
        ui->soundEffectButton->setDisabled(false);
        ui->backgroundMusicButton->setDisabled(false);
        ui->createFinalMovieButton->setDisabled(false);
    }
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

        // Increment the counters
        ui->numberOfFramesLabel->setText (QString::number(_movie->getNumberOfFrames()));
        ui->horizontalSlider->setRange(1,_movie->getNumberOfFrames()+1); // The +1 is because the very last "frame" is the live view
        ui->horizontalSlider->setSliderPosition(_movie->getNumberOfFrames()+1);

        // If the various playback widgets were disabled, enable them
        ui->playButton->setDisabled(false);
        ui->horizontalSlider->setDisabled(false);
        ui->deletePhotoButton->setDisabled(false);
        ui->soundEffectButton->setDisabled(false);
        ui->backgroundMusicButton->setDisabled(false);
        ui->createFinalMovieButton->setDisabled(false);
    }
}

void StopMotionAnimation::on_deletePhotoButton_clicked()
{
    _movie->deleteLastFrame();

    ui->numberOfFramesLabel->setText (QString::number(_movie->getNumberOfFrames()));
    ui->horizontalSlider->setRange(1,_movie->getNumberOfFrames()+1); // The +1 is because the very last "frame" is the live view
    ui->horizontalSlider->setSliderPosition(_movie->getNumberOfFrames()+1);

    if (_movie->getNumberOfFrames() == 0) {
        ui->playButton->setDisabled(true);
        ui->horizontalSlider->setDisabled(true);
        ui->deletePhotoButton->setDisabled(true);
        ui->soundEffectButton->setDisabled(true);
        ui->backgroundMusicButton->setDisabled(true);
        ui->createFinalMovieButton->setDisabled(true);
    }
}

void StopMotionAnimation::on_backgroundMusicButton_clicked()
{
    _backgroundMusic.show();
    // Connect its slot to set the music
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
