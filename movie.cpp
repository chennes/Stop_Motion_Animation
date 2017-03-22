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
#include <iomanip>
#include <thread>
#include <fstream>
#include <sstream>

#include "avcodecwrapper.h"
#include "settings.h"

const qint32 Movie::DEFAULT_FPS (15);

Movie::Movie(const QString &name) :
    _name (name),
    _numberOfFrames (0),
    _currentlyPlaying (false),
    _currentFrame (0),
    _computerSpeedAdjustment (-2)
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

    // Set up libavcodec...
    avcodec_register_all();
}

Movie::~Movie ()
{
    // If we have no frames, delete anything we have saved, including the
    // directory we would have used to store those things.
    if (_numberOfFrames == 0) {
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

QString Movie::getMostRecentFrame () const
{
    return getImageFilename (_numberOfFrames-1);
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
            _soundEffects.append(sfx);
        }
    }
    if (json.contains("backgroundMusic")) {
        QJsonObject backgroundObject (json["backgroundMusic"].toObject());
        _backgroundMusic.load (backgroundObject);
    }
    return true;
}

void Movie::encodeToFile (const QString &filename, const QString &title, const QString &credits) const
{
    Settings settings;
    avcodecWrapper encoder;

    // Video frames first:
    CreatePreTitle(encoder);
    CreateTitle(encoder, title);
    for (int frame = 0; frame < _numberOfFrames; frame++) {
        encoder.AddVideoFrame(getImageFilename(frame));
    }
    CreateCredits(encoder, credits);

    // Audio second:
    if (_backgroundMusic) {
        encoder.AddAudioFile(_backgroundMusic);
    }
    foreach (const SoundEffect &sfx, _soundEffects) {
        encoder.AddAudioFile(sfx);
    }
    int w = settings.Get("settings/imageWidth").toInt();
    int h = settings.Get("settings/imageHeight").toInt();
    QSize resolution (w,h);

    // Consider threading this call if it turns out to take a signficant amount of time...
    try {
        encoder.Encode(filename, resolution.width(), resolution.height(),_framesPerSecond);
    } catch (const avcodecWrapper::libavException &e) {
        throw EncodingFailedException (e.message());
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
        titleFont.setPixelSize (int(0.08 * (double)resolution.height()));
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
        creditsFont.setPixelSize (int(0.05 * (double)h));

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
            distancePerFrame = qreal(textSize.height() - h) / (qreal)(numberOfFrames-(2*numberOfStillFrames));
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
