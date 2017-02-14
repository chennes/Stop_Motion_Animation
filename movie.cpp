#include "movie.h"

#include <QDir>
#include <QImageWriter>
#include <iomanip>
#include <thread>
#include <fstream>
#include <sstream>

#include <iostream>

const unsigned int Movie::DEFAULT_FPS (15);

Movie::Movie(const QString &name) :
    _name (name),
    _numberOfFrames (0),
    _framesPerSecond(DEFAULT_FPS),
    _currentlyPlaying (false),
    _currentFrame (0)
{
    _encoderSettings.setResolution(640,480);
    _encoderSettings.setCodec("JPG");
}

void Movie::setName (const QString &name)
{
    _name = name;
}

QString Movie::getName () const
{
    return _name;
}

void Movie::addFrame (QCamera *camera)
{
    if (camera != _camera) {
        _camera = camera;
        _imageCapture = std::unique_ptr<QCameraImageCapture> (new QCameraImageCapture(camera));
        _imageCapture->setEncodingSettings (_encoderSettings);
    }
    QString filename = getImageFilename (_numberOfFrames);
    _imageCapture->capture (filename);
    _numberOfFrames++;
}

unsigned int Movie::getNumberOfFrames () const
{
    return _numberOfFrames;
}

void Movie::deleteLastFrame ()
{
    if (_numberOfFrames > 0) {
        QString filename = getImageFilename (_numberOfFrames-1) + "." + _encoderSettings.codec().toLower();
        QFile::remove (filename);
        _numberOfFrames--;
    }
}

void Movie::setStillFrame (unsigned int frameNumber, QLabel *video)
{
    if (_currentFrame != frameNumber && frameNumber < _numberOfFrames) {
        _currentFrame = frameNumber;
        QString filename = getImageFilename (_currentFrame);
        video->setPixmap(QPixmap (filename));
        emit frameChanged (frameNumber);
    }
}


void Movie::setFramesPerSecond (unsigned int fps)
{
    _framesPerSecond = fps;
}

unsigned int Movie::getFramesPerSecond () const
{
    return _framesPerSecond;
}

void Movie::setResolution (int w, int h)
{
    if (_numberOfFrames == 0) {
        _encoderSettings.setResolution(w,h);
    } else {
        throw Movie::NoChangesNowException ("Cannot change resolution after frames have been taken.");
    }
}

QSize Movie::getResolution () const
{
    return _encoderSettings.resolution();
}

void Movie::setFormat (const QString &extension)
{
    if (_numberOfFrames == 0) {
        _encoderSettings.setCodec(extension);
    }else {
        throw Movie::NoChangesNowException ("Cannot change file format after frames have been taken.");
    }
}

QString Movie::getFormat () const
{
    return _encoderSettings.codec();
}

QList<QByteArray> Movie::getSupportedFormats () const
{
    return QImageWriter::supportedImageFormats();
}

void Movie::play (unsigned int startFrame, QLabel *video)
{
    if (startFrame >= _numberOfFrames) {
        startFrame = 0;
    }
    _currentlyPlaying = true;
    _frameDestination = video;
    setStillFrame (startFrame, video);
    QObject::connect (&_playbackTimer, &QTimer::timeout, this, &Movie::nextFrame);
    _playbackTimer.setInterval(1000 / _framesPerSecond);
    _playbackTimer.start();
}

void Movie::nextFrame ()
{
    if (_currentlyPlaying && _frameDestination) {
        unsigned int targetFrame = _currentFrame + 1;
        if (targetFrame >= _numberOfFrames) {
            targetFrame = 0;
        }
        setStillFrame (targetFrame, _frameDestination);
    }
}

void Movie::stop ()
{
    _currentlyPlaying = false;
    _playbackTimer.stop();
}

void Movie::addBackgroundMusic (std::shared_ptr<SoundEffect> backgroundMusic)
{
    _backgroundMusic = backgroundMusic;
}

void Movie::addSoundEffect (std::shared_ptr<SoundEffect> soundEffect)
{
    _soundEffects.push_back(soundEffect);
}

void Movie::removeBackgroundMusic()
{
    _backgroundMusic = std::shared_ptr<SoundEffect>();
}

void Movie::removeSoundEffect (std::shared_ptr<SoundEffect> soundEffect)
{
    auto itr = std::find(_soundEffects.begin(), _soundEffects.end(), soundEffect);
    if (itr != _soundEffects.end()) {
        _soundEffects.erase(itr);
    }
}


void Movie::save () const
{
    auto filename = getSaveFilename ();
    std::ofstream s (filename.toStdString());
    if (s.good()) {
        s << _name.toStdString() << "\n"
          << _numberOfFrames << "\n"
          << _framesPerSecond << "\n"
          << _soundEffects.size() << "\n";
        for (auto effect : _soundEffects) {
            s << effect;
        }
        s << _backgroundMusic;
    }
}

void Movie::load ()
{
    auto filename = getSaveFilename ();
    std::ifstream s (filename.toStdString());
    if (s.good()) {
        QString nameFromFile;
        s >> nameFromFile.toStdString();
        if (_name != nameFromFile) {
            // Is this really the correct object to load?
            // throw an exception here
        }
        int nFrames, fps;
        s >> nFrames;
        s >> fps;

        if (nFrames >= 0) {
            _numberOfFrames = (unsigned int) nFrames;
        }
        if (fps >= 1) {
            _framesPerSecond = (unsigned int) fps;
        }

        int numberOfSoundEffects;
        s >> numberOfSoundEffects;
        if (numberOfSoundEffects > 0) {
            _soundEffects.resize(numberOfSoundEffects);
            for (auto&& effect : _soundEffects ) {
                effect = std::shared_ptr<SoundEffect> (new SoundEffect(""));
                s >> *effect;
            }
        }
        _backgroundMusic = std::shared_ptr<SoundEffect> (new SoundEffect(""));
        s >> *_backgroundMusic;
    }
}

void Movie::encodeToFile (const QString &filename) const
{
    // Call ffmpeg
}

QString Movie::getBaseFilename () const
{
    QDir d;
    if (!d.exists("Image Files")) {
        d.mkdir("Image Files");
    }
    d.cd ("Image Files");
    if (!d.exists(_name)) {
        d.mkdir(_name);
    }
    d.cd (_name);
    return d.absoluteFilePath(_name);
}

QString Movie::getSaveFilename () const
{
    QString base = getBaseFilename();
    return base + ".txt";
}

QString Movie::getImageFilename (unsigned int frame) const
{
    std::stringstream ss;
    ss << getBaseFilename().toStdString() << "_" << std::setfill('0') << std::setw(5) << frame;
    return QString::fromStdString(ss.str());
}
