#include "soundeffect.h"

#include <QTimer>
#include <QJsonObject>

#include "settings.h"

SoundEffect::SoundEffect(const QString &filename):
    _filename (filename),
    _startFrame(0),
    _in(0.0),
    _out(0.0),
    _volume(100),
    _playbackEnabled (false),
    _isPlaying (false)
{
}

SoundEffect::SoundEffect(const QString &filename, int startFrame, double in, double out, double volume):
    _filename (filename),
    _startFrame(startFrame),
    _in(in),
    _out(out),
    _volume(volume),
    _playbackEnabled (false),
    _isPlaying (false)
{
}



SoundEffect::SoundEffect(const SoundEffect &sfx) :
    _filename(sfx._filename),
    _startFrame(sfx._startFrame),
    _in(sfx._in),
    _out(sfx._out),
    _volume(sfx._volume),
    _playbackEnabled(false),
    _isPlaying(false)
{
}


SoundEffect& SoundEffect::operator= (const SoundEffect& rhs)
{
    if (this != &rhs) {
        _filename = rhs._filename;
        _startFrame = rhs._startFrame;
        _in = rhs._in;
        _out = rhs._out;
        _volume = rhs._volume;
        _isPlaying = false;
        _playbackEnabled = false;
    }
    return *this;
}

bool SoundEffect::operator== (const SoundEffect& rhs) const
{
    // Here we treat the sound effect as identical if it starts at the same frame and has the
    // same filename. The other quantities are floating point numbers and should not be used
    // in comparisons here due to rounding in the user interface.
    return (_filename == rhs._filename &&
            _startFrame == rhs._startFrame);
}

bool SoundEffect::operator< (const SoundEffect& rhs) const
{
    if (_startFrame == rhs._startFrame) {
        return _filename < rhs._filename;
    } else {
        return _startFrame < rhs._startFrame;
    }
}

SoundEffect::operator bool() const
{
    return (_filename.length()>0);
}

void SoundEffect::setStartTime (double t)
{
    Settings settings;
    int fps = settings.Get ("settings/framesPerSecond").toInt();
    _startFrame = int(round(t * fps));
}

void SoundEffect::setStartFrame(int f)
{
    _startFrame = f;
}


void SoundEffect::setInPoint (double t)
{
    _in = t;
}


void SoundEffect::setOutPoint (double t)
{
    _out = t;
}


void SoundEffect::setVolume (double v)
{
    _volume = v;
    if (_playbackEnabled) {
        _playback->setVolume(_volume);
    }
}



QString SoundEffect::getFilename () const
{
    return _filename;
}

double SoundEffect::getStartTime () const
{
    Settings settings;
    int fps = settings.Get ("settings/framesPerSecond").toInt();
    return (double)_startFrame / (double)fps;
}

int SoundEffect::getStartFrame() const
{
    return _startFrame;
}

double SoundEffect::getInPoint () const
{
    return _in;
}


double SoundEffect::getOutPoint () const
{
    return _out;
}


double SoundEffect::getVolume () const
{
    return _volume;
}


void SoundEffect::mediaStatusChanged (QMediaPlayer::MediaStatus s)
{
    qDebug() << "Playback status changed to " << s;
    switch (s) {
    case QMediaPlayer::LoadedMedia:
        if (_out == 0.0) {
            _out = _playback->duration() / 1000;
        }
        break;
    default:
        break;
    }
}

void SoundEffect::enablePlayback()
{
    if (!_playbackEnabled) {
        _playback = new QMediaPlayer;
        connect (_playback, &QMediaPlayer::mediaStatusChanged,
                 this, &SoundEffect::mediaStatusChanged);
        if (_filename.length() > 0) {
            _playback->setMedia (QUrl::fromLocalFile(_filename));
        }
    }
    _playbackEnabled = true;
}

void SoundEffect::play () const
{
    playFrom(0.0);
}


void SoundEffect::playFrom (double t) const
{
    if (!_playbackEnabled) {
        qDebug() << "Playback was disabled!";
        return;
    }
    if (_out > _in || _out == 0.0) {
        _isPlaying = true;
        _playback->setPosition(1000 * (t+_in));
        _playback->setVolume(_volume);
        _playback->play();
        if (_out != 0.0) {
            QTimer::singleShot (int(1000*(_out-_in)), this, SLOT(stop()));
        }
    }
}


void SoundEffect::stop () const
{
    if (_playbackEnabled && _isPlaying) {
        _isPlaying = false;
        _playback->stop();
    }
}

void SoundEffect::load (const QJsonObject &json)
{
    _filename = json["filename"].toString();
    _startFrame = json["startFrame"].toInt();
    _in = json["in"].toDouble();
    _out = json["out"].toDouble();
    _volume = json["volume"].toDouble();
    if (_volume == 0) {
        _volume = 1;
    }

    if (_filename.length() > 0 && _playbackEnabled) {
        _playback->setMedia (QUrl::fromLocalFile(_filename));
    }
}

void SoundEffect::save (QJsonObject &json) const
{
    json["filename"] = _filename;
    json["startFrame"] = _startFrame;
    json["in"] = _in;
    json["out"] = _out;
    json["volume"] = _volume;
}
