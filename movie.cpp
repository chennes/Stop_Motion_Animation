#include "movie.h"

#include <QDir>
#include <QImageWriter>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QImageEncoderControl>
#include <QProcess>
#include <iomanip>
#include <thread>
#include <fstream>
#include <sstream>

#include "avcodecwrapper.h"

const qint32 Movie::DEFAULT_FPS (15);

Movie::Movie(const QString &name) :
    _name (name),
    _numberOfFrames (0),
    _currentlyPlaying (false),
    _currentFrame (0),
    _computerSpeedAdjustment (-2)
{
    QSettings settings;

    QSize resolution = settings.value("settings/resolution",QSize(640,480)).toSize();
    _encoderSettings.setResolution(resolution);

    // Note that this doesn't currently work: the cameras we are using ONLY support saving to JPG files
    // directly, so if a different format is required we'd have to convert it. We don't.
    QString format = settings.value("settings/imageFileType","JPG").toString();
    //_encoderSettings.setCodec(format);
    _encoderSettings.setCodec("JPG"); // This at least lets us prepare for a future feature that DOES support other formats

    qint32 framesPerSecond = settings.value("settings/framesPerSecond",Movie::DEFAULT_FPS).toInt();
    _framesPerSecond = framesPerSecond;

    // Set up libavcodec...
    avcodec_register_all();
}

Movie::~Movie ()
{
    // If we have no frames, delete anything we have saved, including the
    // directory we would have used to store those things.
    if (_numberOfFrames == 0) {
        QDir d;
        QSettings settings;
        QString imageStorageLocation = settings.value("settings/imageStorageLocation","Image Files/").toString();
        if (d.exists(imageStorageLocation)) {
            d.cd (imageStorageLocation);
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
        if (_imageCapture->error() != QCameraImageCapture::NoError) {
            throw CaptureFailedException ("Image capture failed: " + _imageCapture->errorString());
        }
    }
    QString filename = getImageFilename (_numberOfFrames);
    if (_imageCapture->isReadyForCapture()) {
        _imageCapture->capture (filename);
        if (_imageCapture->error() != QCameraImageCapture::NoError) {
            throw CaptureFailedException ("Image capture failed: " + _imageCapture->errorString());
        }
        _numberOfFrames++;
    }
}

void Movie::importFrame (const QString &filename)
{
    QString newFilename = getImageFilename (_numberOfFrames) + "." + _encoderSettings.codec().toLower();
    bool success = QFile::copy (filename, newFilename);
    if (success) {
        _numberOfFrames++;
    } else {
        throw Movie::ImportFailedException ();
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

void Movie::play (qint32 startFrame, QLabel *video)
{
    if (startFrame >= _numberOfFrames) {
        startFrame = 0;
    }
    _currentlyPlaying = true;
    _frameDestination = video;
    _playFrameCounter = 1;
    _playStartTime.start();
    _lastFrameTime.start();
    setStillFrame (startFrame, video);
    QObject::connect (&_playbackTimer, &QTimer::timeout, this, &Movie::nextFrame);
    _playbackTimer.setInterval(1000 / _framesPerSecond + _computerSpeedAdjustment);
    _playbackTimer.start();
}

void Movie::nextFrame ()
{
    if (_currentlyPlaying && _frameDestination) {
        int frameAdjust = 0;

        _playFrameCounter++;
        int elapsedPlayTime = _playStartTime.elapsed();
        int millisPerFrame = 1000 / _framesPerSecond;
        int expectedMillis = _playFrameCounter * millisPerFrame;

        // Tweak the timer to make sure we are playing back at as close to the expected FPS as
        // possible. This is important if we are playing along with music.
        qint32 nominalMS = (1000 / _framesPerSecond);
        if (_lastFrameTime.elapsed() != nominalMS) {
            _computerSpeedAdjustment = nominalMS - _lastFrameTime.elapsed();
            _playbackTimer.setInterval(1000 / _framesPerSecond + _computerSpeedAdjustment);
        }
        _lastFrameTime.restart();

        // Worst case scenario, we can drop frames to catch up.
        if (expectedMillis < elapsedPlayTime-millisPerFrame) {
            // Skip a frame.
            _playFrameCounter++;
            frameAdjust = 1;
        }

        qint32 targetFrame = _currentFrame + 1 + frameAdjust;
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
    avcodecWrapper encoder;
    for (int frame = 0; frame < _numberOfFrames; frame++) {
        encoder.AddVideoFrame(getImageFilename(frame));
    }
    if (_backgroundMusic) {
        encoder.AddAudioFile(_backgroundMusic);
    }
    foreach (const SoundEffect &sfx, _soundEffects) {
        encoder.AddAudioFile(sfx);
    }
    QSettings settings;
    QSize resolution = settings.value("settings/resolution",QSize(640,480)).toSize();

    // Consider threading this call if it turns out to take a signficant amount of time...
    try {
        encoder.Encode(filename, resolution.width(), resolution.height(),_framesPerSecond);
    } catch (const avcodecWrapper::libavException &e) {
        throw EncodingFailedException (e.message());
    }
}

//std::shared_ptr<QProcess> Movie::encodeToFile (const QString &filename) const
//{
//    // Construct the call to FFmpeg...
//    QStringList ffmpegArguments;

//    // "Input" frame rate (which we will tell FFmpeg matches our output frame rate)
//    ffmpegArguments.push_back("-r " + QString::number(_framesPerSecond));

//    // Our image filename pattern is (filename)_#####.(extension)
//    QString filenamePattern (getBaseFilename() + "_%5d");
//    ffmpegArguments.push_back("-i " + filenamePattern);

//    // TODO: Audio stuff will go here...

//    // "Output" frame rate, same as input
//    ffmpegArguments.push_back("-r " + QString::number(_framesPerSecond));

//    // Set the output duration so that if there is audio extending after the end of the video
//    // it gets cut off (instead of just playing with black frames).
//    float duration = _numberOfFrames / float(_framesPerSecond);
//    ffmpegArguments.push_back("-t " + QString::number(duration));

//    // Make sure FFmpeg never waits for stdin:
//    ffmpegArguments.push_back("-nostdin");

//    // Use an older video format so that Windows Media Player can play it
//    ffmpegArguments.push_back("-pix_fmt yuv420p");

//    QSettings settings;
//    QString pathToFFmpeg = settings.value("settings/pathToFFmpeg", "").toString();
//    if (pathToFFmpeg == "") {
//        if (QSysInfo::currentCpuArchitecture() == "x86_64") {
//            QString system = QSysInfo::productType();
//            if (system == "windows") {
//                pathToFFmpeg = "./data/windows/ffmpeg/ffmpeg.exe";
//            } else if (system == "macos") {
//                pathToFFmpeg = "./data/macos/ffmpeg/ffmpeg";
//            } else {
//                // Something else, try to auto-detect a system-installed ffmpeg
//            }
//        } else {
//            // Not a current standard (in 2017) computer CPU, try to auto-detect a system-installed ffmpeg
//        }
//    }

//    std::shared_ptr<QProcess> ffmpegProcess (new QProcess);
//    ffmpegProcess->setProgram(pathToFFmpeg);
//    ffmpegProcess->setArguments(ffmpegArguments);
//    QString timestamp (QString::number(QDateTime::currentSecsSinceEpoch()));
//    ffmpegProcess->setStandardOutputFile(getBaseFilename() + "ffmpeg_" + timestamp + ".out");
//    ffmpegProcess->setStandardErrorFile(getBaseFilename() + "ffmpeg_" + timestamp + ".err");
//    ffmpegProcess->start();

//    return ffmpegProcess;
//}

QString Movie::getBaseFilename () const
{
    QDir d;
    QSettings settings;
    QString imageStorageLocation = settings.value("settings/imageStorageLocation","Image Files/").toString();

    if (!d.exists(imageStorageLocation)) {
        d.mkdir(imageStorageLocation);
    }
    d.cd (imageStorageLocation);
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
    ss << getBaseFilename().toStdString() << "_" << std::setfill('0') << std::setw(5) << frame << "." << _encoderSettings.codec().toLower().toStdString();
    return QString::fromStdString(ss.str());
}
