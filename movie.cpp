#include "movie.h"

#include <QDir>
#include <QImageWriter>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <iomanip>
#include <thread>
#include <fstream>
#include <sstream>

#include <iostream>

const qint32 Movie::DEFAULT_FPS (15);

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

Movie::~Movie ()
{
    // If we have no frames, delete anything we have saved, including the
    // directory we would have used to store those things.
    if (_numberOfFrames == 0) {
        QDir d;
        if (d.exists("Image Files")) {
            d.cd ("Image Files");
            if (d.exists(_name)) {
                d.cd (_name);
                d.removeRecursively();
            }
        }
    }
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

void Movie::importFrame (const QString &filename)
{
    QString newFilename = getImageFilename (_numberOfFrames) + "." + _encoderSettings.codec().toLower();
    bool success = QFile::copy (filename, newFilename);
    if (success) {
        _numberOfFrames++;
    } else {
        std::cout << "FAILED: \n"
                  << filename.toStdString() << "\n"
                  << newFilename.toStdString() << std::endl;
        throw ImportFailedException ();
    }
}

qint32 Movie::getNumberOfFrames () const
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

void Movie::setStillFrame (qint32 frameNumber, QLabel *video)
{
    if (_currentFrame != frameNumber && frameNumber < _numberOfFrames) {
        _currentFrame = frameNumber;
        QString filename = getImageFilename (_currentFrame);
        video->setPixmap(QPixmap (filename));
        emit frameChanged (frameNumber);
    }
}


void Movie::setFramesPerSecond (qint32 fps)
{
    _framesPerSecond = fps;
}

qint32 Movie::getFramesPerSecond () const
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

void Movie::play (qint32 startFrame, QLabel *video)
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
        qint32 targetFrame = _currentFrame + 1;
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

void Movie::addBackgroundMusic (const SoundEffect &backgroundMusic)
{
    _backgroundMusic = backgroundMusic;
}

void Movie::addSoundEffect (const SoundEffect &soundEffect)
{
    _soundEffects.push_back(soundEffect);
}

void Movie::removeBackgroundMusic()
{
    _backgroundMusic = SoundEffect();
}

void Movie::removeSoundEffect (const SoundEffect &soundEffect)
{
    auto itr = std::find(_soundEffects.begin(), _soundEffects.end(), soundEffect);
    if (itr != _soundEffects.end()) {
        _soundEffects.erase(itr);
    }
}


void Movie::save () const
{
    if (_numberOfFrames > 0) {
        auto filename = getSaveFilename ();
        QFile saveFile (filename);
        bool isOpen = saveFile.open(QIODevice::WriteOnly);
        if (!isOpen) {
            throw Movie::FailedToSaveException(filename);
        }
        QJsonObject json;
        json["name"] = _name;
        json["numberOfFrames"] = _numberOfFrames;
        json["framesPerSecond"] = _framesPerSecond;

        QJsonArray sfxArray;
        foreach (const SoundEffect sfx, _soundEffects) {
            QJsonObject sfxObject;
            sfx.save (sfxObject);
            sfxArray.append(sfxObject);
        }
        json["sfx"] = sfxArray;

        QJsonObject backgroundObject;
        _backgroundMusic.save (backgroundObject);

        json["backgroundMusic"] = backgroundObject;

        QJsonDocument jsonDocument (json);
        saveFile.write(jsonDocument.toJson());
        saveFile.close();
    }
}

bool Movie::load (const QString &filename)
{
    QFile loadFile (filename);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray saveData = loadFile.readAll();
    QJsonDocument jsonDocument (QJsonDocument::fromJson(saveData));
    QJsonObject json (jsonDocument.object());
    _name = json["name"].toString();
    _numberOfFrames = json["numberOfFrames"].toInt();
    _framesPerSecond = json["framesPerSecond"].toInt();

    QJsonArray sfxArray = json["sfx"].toArray();
    _soundEffects.clear();
    for (int sfxIndex = 0; sfxIndex < sfxArray.size(); ++sfxIndex) {
        QJsonObject sfxObject (sfxArray[sfxIndex].toObject());
        SoundEffect sfx;
        sfx.load (sfxObject);
        _soundEffects.append(sfx);
    }
    QJsonObject backgroundObject (json["backgroundMusic"].toObject());
    _backgroundMusic.load (backgroundObject);
    return true;
}

void Movie::encodeToFile (const QString &filename) const
{
    // Call ffmpeg... or something. Maybe Qt can just encode in-house?
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
    return base + ".json";
}

QString Movie::getImageFilename (qint32 frame) const
{
    std::stringstream ss;
    ss << getBaseFilename().toStdString() << "_" << std::setfill('0') << std::setw(5) << frame;
    return QString::fromStdString(ss.str());
}
