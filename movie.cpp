#include "movie.h"

#include <QDir>
#include <QImageWriter>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QImageEncoderControl>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QtConcurrent/QtConcurrent>
#include <iomanip>
#include <thread>
#include <fstream>
#include <sstream>
#include <functional>

#include "avcodecwrapper.h"
#include "plsexception.h"
#include "settings.h"
#include "utils.h"

Movie::Movie(const QString &name, bool allowModifications) :
    _name (name),
    _numberOfFrames (0),
    _allowModifications (allowModifications),
    _currentlyPlaying (false),
    _currentFrame (-1),
    _skippedFrameCounter(0),
    _computerSpeedAdjust (0),
    _mute (false)
{
    Settings settings;

    int w = settings.Get("settings/imageWidth").toInt();
    int h = settings.Get("settings/imageHeight").toInt();
    QSize resolution (w,h);
    _encoderSettings.setResolution(resolution);

    // Note that this doesn't currently work: the cameras we are using ONLY support saving to JPG files
    // directly, so if a different format is required we'd have to convert it. We don't.
    QString format = settings.Get("settings/imageFileType").toString();
    //_encoderSettings.setCodec(format);
    _encoderSettings.setCodec("JPG"); // This at least lets us prepare for a future feature that DOES support other formats

    qint32 framesPerSecond = settings.Get("settings/framesPerSecond").toInt();
    _framesPerSecond = framesPerSecond;

    QObject::connect (&_playbackTimer, &QTimer::timeout, this, &Movie::nextFrame);
}

Movie::~Movie ()
{
    // If we have no frames, delete anything we have saved, including the
    // directory we would have used to store those things.
    if (_numberOfFrames == 0 && _allowModifications) {
        QDir d;
        Settings settings;
        QString imageStorageLocation = settings.Get("settings/imageStorageLocation").toString();
        if (d.exists(imageStorageLocation)) {
            d.cd (imageStorageLocation);
            if (d.exists(_name)) {
                d.cd (_name);
                d.removeRecursively();
            }
        }
    }

    for (auto tempFile: _encodingTempFiles) {
        QFile f (tempFile);
        f.remove();
    }
}

void Movie::setName (const QString &name)
{
    if (!_allowModifications) {
        throw NoChangesNowException ("Cannot change movie name when it is locked");
    }
    _name = name;
}

QString Movie::getName () const
{
    return _name;
}



QString Movie::getEncodingFilename() const
{
    if (_encodingFilename.length() == 0) {
        Settings settings;
        QString imageStorageLocation = settings.Get("settings/imageStorageLocation").toString();
        QString newEncodingFilename = imageStorageLocation + _name + ".mp4";
        return newEncodingFilename;
    } else {
        return _encodingFilename;
    }
}

QString Movie::getEncodingTitle() const
{
    return _encodingTitle;
}

QString Movie::getEncodingCredits() const
{
    return _encodingCredits;
}

void Movie::setCamera (QCamera *camera)
{
    if (camera != _camera) {
        _camera = camera;
        _imageCapture = std::unique_ptr<QCameraImageCapture> (new QCameraImageCapture(camera));
        _imageCapture->setEncodingSettings (_encoderSettings);
        if (_imageCapture->error() != QCameraImageCapture::NoError) {
            throw CaptureFailedException ("Image capture failed: " + _imageCapture->errorString());
        }
        connect (_imageCapture.get(), &QCameraImageCapture::readyForCaptureChanged,
                 this, &Movie::readyForCaptureChanged);
        connect (_imageCapture.get(), &QCameraImageCapture::imageSaved,
                 this, &Movie::imageSaved);
    }
}

void Movie::readyForCaptureChanged(bool)
{
}

void Movie::imageSaved (int, const QString &fileName)
{
    if (_fileForRotation.length() > 0 &&
        _fileForRotation == fileName) {
        // Fire off a process to rotate the image in the background.
        QtConcurrent::run(&rotateImageFile,_fileForRotation);
        _fileForRotation = "";
    }
}


void Movie::addFrame (bool rotate180)
{
    if (!_allowModifications) {
        throw NoChangesNowException ("Cannot add frame to movie when it is locked");
    }
    QString filename = getImageFilename (_numberOfFrames);
    if (_imageCapture->isReadyForCapture()) {
        _imageCapture->capture (filename);
        if (rotate180) {
            _fileForRotation = filename;
        } else {
            _fileForRotation = "";
        }
        if (_imageCapture->error() != QCameraImageCapture::NoError) {
            throw CaptureFailedException ("Image capture failed: " + _imageCapture->errorString());
        }
        _numberOfFrames++;
        save();
    } else {
        // Not ready for capture?
        qDebug() << "Not ready to capture!";
    }
}

void Movie::importFrame (const QString &filename)
{
    if (!_allowModifications) {
        throw NoChangesNowException ("Cannot add frame to movie when it is locked");
    }
    QString newFilename = getImageFilename (_numberOfFrames);
    bool success = QFile::copy (filename, newFilename);
    if (success) {
        _numberOfFrames++;
        save();
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
    if (!_allowModifications) {
        throw NoChangesNowException ("Cannot remove frame from movie when it is locked");
    }
    if (_numberOfFrames > 0) {
        QString filename = getImageFilename (_numberOfFrames-1) + "." + _encoderSettings.codec().toLower();
        QFile::remove (filename);
        _numberOfFrames--;
        save();
    }
}

QString Movie::getMostRecentFrame () const
{
    return getImageFilename (_numberOfFrames-1);
}

void Movie::playSoundEffect(const SoundEffect &sfx, QLabel *video)
{
    _mute = true;
    auto memberSFX = std::find(_soundEffects.begin(), _soundEffects.end(), sfx);
    if (memberSFX == _soundEffects.end()) {
        throw std::runtime_error ("Internal error: sound effect not found in movie.");
    }
    auto startFrame = memberSFX->getStartFrame();
    auto duration = sfx.getOutPoint()-sfx.getInPoint();
    if (duration*_framesPerSecond + startFrame > _numberOfFrames) {
        // Correct for durations that extend past the end of the current movie.
        duration = double(_numberOfFrames-startFrame-1)/_framesPerSecond;
        qDebug() << "Playing SFX for " << duration << " seconds";
    }
    play (startFrame, video);
    memberSFX->play();
    QTimer::singleShot (int(1000*duration), this, &Movie::stop);
    QTimer::singleShot (int(1000*duration)+10, this, std::bind(&Movie::setStillFrame, this, startFrame, video));
}

void Movie::setStillFrame (qint32 frameNumber, QLabel *video)
{
    if (frameNumber < _numberOfFrames) {
        _currentFrame = frameNumber;
        QString filename = getImageFilename (_currentFrame);
        QPixmap pix (filename);
        video->setPixmap(pix);
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
    _skippedFrameCounter = 0;
    _playStartTime.start();
    setStillFrame (startFrame, video);
    if (_backgroundMusic && !_mute) {
        Settings settings;
        double tOffset = settings.Get("settings/preTitleScreenDuration").toDouble() +
                         settings.Get("settings/titleScreenDuration").toDouble();
        double startTime = tOffset + double(startFrame) / double(_framesPerSecond);
        _backgroundMusic.playFrom(startTime);
    }
    int interval = (1000 / _framesPerSecond) - _computerSpeedAdjust;
    qDebug() << "Frame interval: " << interval << "ms";
    _playbackTimer.setInterval(interval);
    _playbackTimer.start();
}

void Movie::nextFrame ()
{
    if (_currentlyPlaying && _frameDestination) {
        int frameAdjust { 0 };

        _playFrameCounter++;
        int elapsedPlayTime {_playStartTime.elapsed()};
        double millisPerFrame {1000.0 / _framesPerSecond};
        int expectedMillis {int(round(_playFrameCounter * millisPerFrame))};

        // If we have to, we can drop frames to catch up.
        if (expectedMillis < elapsedPlayTime-millisPerFrame) {
            // Skip a frame.
            _playFrameCounter++;
            _skippedFrameCounter++;
            frameAdjust = 1;
        }

        qint32 targetFrame = _currentFrame + 1 + frameAdjust;
        if (targetFrame >= _numberOfFrames) {
            stop();
            play(0, _frameDestination);
            return;
        } else {
            setStillFrame (targetFrame, _frameDestination);
            if (frameAdjust > 0) {
                for (int adjustment = frameAdjust; adjustment >= 1; adjustment--) {
                    if (!_mute && _soundEffects.contains(targetFrame-adjustment)) {
                        _soundEffects[targetFrame-adjustment].play();
                    }
                }
            }
            if (!_mute && _soundEffects.contains(targetFrame)) {
                _soundEffects[targetFrame].play();
            }

        }
    }
}

void Movie::stop ()
{
    _currentlyPlaying = false;
    _mute = false;
    _playbackTimer.stop();
    _backgroundMusic.stop();
    for (auto &&sfx: _soundEffects) {
        sfx.stop();
    }
    qDebug() << "Skipped " << _skippedFrameCounter << " frames in that run.";

    int elapsedPlayTime {_playStartTime.elapsed()};
    double millisPerFrame {1000.0 / _framesPerSecond};
    int expectedMillis {int(round((_playFrameCounter-_skippedFrameCounter) * millisPerFrame))};

    // Tweak the playback timer speed to adjust for the computer's speed.
    if (elapsedPlayTime > expectedMillis + millisPerFrame) {
        _computerSpeedAdjust++;
        _computerSpeedAdjust = std::min (_computerSpeedAdjust, 10); // Don't let it get too big
    } else if (elapsedPlayTime < expectedMillis - millisPerFrame) {
        _computerSpeedAdjust--;
        _computerSpeedAdjust = std::max (_computerSpeedAdjust, -10);// ... or too small
    }

}

void Movie::addBackgroundMusic (const SoundEffect &backgroundMusic)
{
    if (!_allowModifications) {
        throw NoChangesNowException ("Cannot add music to movie when it is locked");
    }
    _backgroundMusic = backgroundMusic;
    _backgroundMusic.enablePlayback();
    save();
}

void Movie::addSoundEffect (const SoundEffect &soundEffect)
{
    if (!_allowModifications) {
        throw NoChangesNowException ("Cannot add sound effect to movie when it is locked");
    }
    if (!soundEffect) {
        // We are really removing the current frame's SFX
        if (_currentFrame >= 0 && _currentFrame < _numberOfFrames) {
            if (_soundEffects.contains(_currentFrame)) {
                _soundEffects.remove(_currentFrame);
            }
        }
    } else {
        _soundEffects.insert(_currentFrame, soundEffect);
        _soundEffects[_currentFrame].setStartFrame(_currentFrame);
        _soundEffects[_currentFrame].enablePlayback();
    }
    save();
}

void Movie::removeBackgroundMusic()
{
    if (!_allowModifications) {
        throw NoChangesNowException ("Cannot change movie when it is locked");
    }
    _backgroundMusic = SoundEffect();
    save();
}

SoundEffect Movie::getBackgroundMusic () const
{
    return _backgroundMusic;
}

SoundEffect Movie::getSoundEffect (int frame) const
{
    if (_soundEffects.contains(frame)) {
        return _soundEffects[frame];
    } else {
        return SoundEffect();
    }
}

QList<SoundEffect> Movie::getSoundEffects () const
{
    QList<SoundEffect> returnedSFX;
    for (auto &&sfx:_soundEffects) {
        returnedSFX.append(sfx);
    }
    return returnedSFX;
}

void Movie::removeSoundEffect (const SoundEffect &soundEffect)
{
    if (!_allowModifications) {
        throw NoChangesNowException ("Cannot change movie when it is locked");
    }
    auto itr = std::find(_soundEffects.begin(), _soundEffects.end(), soundEffect);
    if (itr != _soundEffects.end()) {
        _soundEffects.erase(itr);
        save();
    }
}


void Movie::save () const
{
    if (_numberOfFrames > 0 && _allowModifications) {
        auto filename = getSaveFilename ();
        QJsonObject json;
        json["name"] = _name;
        json["numberOfFrames"] = _numberOfFrames;
        json["framesPerSecond"] = _framesPerSecond;

        QJsonArray sfxArray;
        for (auto sfx: _soundEffects) {
            QJsonObject sfxObject;
            sfx.save (sfxObject);
            sfxArray.append(sfxObject);
        }
        json["sfx"] = sfxArray;

        QJsonObject backgroundObject;
        _backgroundMusic.save (backgroundObject);

        json["backgroundMusic"] = backgroundObject;


        json["encodingFilename"] = _encodingFilename;
        json["encodingTitle"] = _encodingTitle;
        json["encodingCredits"] = _encodingCredits;

        QJsonDocument jsonDocument (json);
        QFile saveFile (filename);
        bool isOpen = saveFile.open(QIODevice::WriteOnly);
        if (!isOpen) {
            throw Movie::FailedToSaveException(filename);
        }
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

    //At a minimum, this file must have the following elements in it:
    if (json.contains("name")) {
        _name = json["name"].toString();
    } else {
        return false;
    }
    if (json.contains("numberOfFrames")) {
     _numberOfFrames = json["numberOfFrames"].toInt();
    } else {
        return false;
    }
    if (json.contains("framesPerSecond")) {
        _framesPerSecond = json["framesPerSecond"].toInt();
    } else {
        return false;
    }

    // Sound effects are optional:
    if (json.contains("sfx")) {
        QJsonArray sfxArray = json["sfx"].toArray();
        _soundEffects.clear();
        for (int sfxIndex = 0; sfxIndex < sfxArray.size(); ++sfxIndex) {
            QJsonObject sfxObject (sfxArray[sfxIndex].toObject());
            SoundEffect sfx;
            sfx.load (sfxObject);
            if (sfx) {
                _soundEffects.insert(sfx.getStartFrame(), sfx);
                _soundEffects[sfx.getStartFrame()].enablePlayback();
            }
        }
    }
    if (json.contains("backgroundMusic")) {
        QJsonObject backgroundObject (json["backgroundMusic"].toObject());
        _backgroundMusic.load (backgroundObject);
        if (_backgroundMusic) {
            _backgroundMusic.enablePlayback();
        }
    }

    _encodingFilename = json["encodingFilename"].toString();
    _encodingTitle = json["encodingTitle"].toString();
    _encodingCredits = json["encodingCredits"].toString();
    return true;
}

void Movie::encodeToFile (const QString &filename, const QString &title, const QString &credits)
{
    Settings settings;
    avcodecWrapper encoder;

    _encodingFilename = filename;
    _encodingTitle = title;
    _encodingCredits = credits;

    save();

    for (auto tempFile: _encodingTempFiles) {
        QFile f (tempFile);
        f.remove();
    }
    _encodingTempFiles.clear();

    // Video frames first:
    CreatePreTitle(encoder);
    CreateTitle(encoder, title);
    for (int frame = 0; frame < _numberOfFrames; frame++) {
        encoder.AddVideoFrame(getImageFilename(frame));
    }
    CreateCredits(encoder, credits);

    // Audio second:
    if (_backgroundMusic) {
        encoder.AddAudioFile(_backgroundMusic, 0);
    }

    double ptsDuration = settings.Get("settings/preTitleScreenDuration").toDouble();
    double tsDuration = settings.Get("settings/titleScreenDuration").toDouble();

    for (auto sfx: _soundEffects) {
        if (!sfx) {
            qDebug() << "Empty SFX found!";
            continue;
        }
        encoder.AddAudioFile(sfx, ptsDuration+tsDuration);
    }
    int w = settings.Get("settings/imageWidth").toInt();
    int h = settings.Get("settings/imageHeight").toInt();
    QSize resolution (w,h);

    // Consider threading this call if it turns out to take a signficant amount of time...
    try {
        encoder.Encode(filename, resolution.width(), resolution.height(),_framesPerSecond);
    } catch (const avcodecWrapper::libavException &e) {
        throw EncodingFailedException (e.message());
    } catch (const PLSException &e) {
        throw EncodingFailedException (e.message());
    } catch (...) {
        throw EncodingFailedException ("The encoding failed with an unrecognized error");
    }
}


void Movie::CreatePreTitle(avcodecWrapper &encoder) const
{
    Settings settings;
    QString filename = settings.Get("settings/preTitleScreenLocation").toString();
    double duration = settings.Get("settings/preTitleScreenDuration").toDouble();
    if (duration > 0 && filename != "") {
        QFile preTitleScreenCheck (filename);
        if (!preTitleScreenCheck.exists()) {
            throw EncodingFailedException ("Pre-Title screen could not be opened: " + filename);
        }
        int numberOfFrames = int(std::round(_framesPerSecond * duration));
        for (int frame = 0; frame < numberOfFrames; frame++) {
            // Just blindly add the file over and over again. TODO: Someday consider checking it for
            // validity first.
            encoder.AddVideoFrame(filename);
        }
    }
}

void Movie::CreateTitle(avcodecWrapper &encoder, const QString &title) const
{
    Settings settings;
    double duration = settings.Get("settings/titleScreenDuration").toDouble();
    if (duration > 0 && title != "") {
        int w = settings.Get("settings/imageWidth").toInt();
        int h = settings.Get("settings/imageHeight").toInt();
        QSize resolution (w,h);
        QGraphicsScene scene;
        scene.setSceneRect(0,0,resolution.width(),resolution.height());
        scene.setBackgroundBrush(Qt::black);
        QFont titleFont;
        titleFont.setBold(true);
        titleFont.setPixelSize (int(0.08 * double(resolution.height())));
        QGraphicsTextItem *titleTextItem = scene.addText ("title", titleFont);
        titleTextItem->setHtml("<center>" + title + "</center>");
        titleTextItem->setDefaultTextColor(Qt::white);
        titleTextItem->setTextWidth(0.8*resolution.width());

        QRectF textSize = titleTextItem->boundingRect();
        QPointF textPosition (0.1 * resolution.width(),(resolution.height() - textSize.height())/2);
        titleTextItem->setPos (textPosition);

        QImage img(resolution.width(), resolution.height(), QImage::Format_ARGB32);
        QPainter painter;
        painter.begin(&img);
        scene.render(&painter);
        painter.end();

        QString titleScreenFilename = getBaseFilename() + "_titleScreen.jpg";
        img.save(titleScreenFilename);
        _encodingTempFiles.append(titleScreenFilename);
        int numberOfFrames = int(std::round(_framesPerSecond * duration));
        for (int frame = 0; frame < numberOfFrames; frame++) {
            encoder.AddVideoFrame(titleScreenFilename);
        }
    }
}

void Movie::CreateCredits(avcodecWrapper &encoder, const QString &credits) const
{   
    Settings settings;
    double duration = settings.Get("settings/creditsDuration").toDouble();
    if (duration > 0 && credits != "") {
        int w = settings.Get("settings/imageWidth").toInt();
        int h = settings.Get("settings/imageHeight").toInt();
        QGraphicsScene scene;
        scene.setSceneRect(0,0,w,h);
        scene.setBackgroundBrush(Qt::black);
        QFont creditsFont;
        creditsFont.setPixelSize (int(0.05 * double(h)));

        QGraphicsTextItem *titleTextItem = scene.addText ("", creditsFont);

        // The credits are coming in in plain text, but we really want them to be in HTML:
        // in particular, we need to replace \n with <br/>.
        QString htmlCredits (credits);
        htmlCredits.replace('\n',"<br/>");


        QDate today;
        titleTextItem->setHtml("<center><h1>Credits</h1></center><p>" + htmlCredits + "</p><br/><p>Created " + today.toString() + " using Pioneer Library System's Stop Motion Creator software.</p>");
        titleTextItem->setDefaultTextColor(Qt::white);
        titleTextItem->setTextWidth(0.8*w);

        QRectF textSize = titleTextItem->boundingRect();

        int numberOfFrames = int(std::round(_framesPerSecond * duration));
        int numberOfStillFrames = numberOfFrames / 10;

        // See if we need to scroll the credits:
        qreal distancePerFrame = 0;
        if (textSize.height() > h) {
            distancePerFrame = qreal(textSize.height() - h) / qreal(numberOfFrames-(2*numberOfStillFrames));
        }

        qreal verticalPosition = 0;
        for (int frame = 0; frame < numberOfFrames; frame++) {
            if (frame > numberOfStillFrames && frame < numberOfFrames-numberOfStillFrames) {
                verticalPosition -= distancePerFrame;
            }
            QPointF textPosition (0.1 * w,verticalPosition);
            titleTextItem->setPos(textPosition);

            QImage img(w, h, QImage::Format_ARGB32);
            QPainter painter;
            painter.begin(&img);
            scene.render(&painter);
            painter.end();

            QString titleScreenFilename = getBaseFilename() + "_credits_" + QString::number(frame) + ".jpg";
            _encodingTempFiles.append (titleScreenFilename);
            img.save(titleScreenFilename);
            encoder.AddVideoFrame(titleScreenFilename);
        }
    }
}

QString Movie::getBaseFilename () const
{
    QDir d;
    Settings settings;
    QString imageStorageLocation = settings.Get("settings/imageStorageLocation").toString();

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
