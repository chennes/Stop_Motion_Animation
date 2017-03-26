#include "soundeffect.h"

#include <QTimer>
#include <QJsonObject>

#include "settings.h"

SoundEffect::SoundEffect(const QString &filename):
    _filename (filename),
    _startFrame(0),
    _in(0.0),
    _volume(1.0),
    _isPlaying (false)
{
    if (_filename.length() > 0) {
        _playback.setMedia (QUrl::fromLocalFile(filename));
        _out = double(_playback.duration()) / 1000.0;
    }
}

SoundEffect::SoundEffect(const QString &filename, int startFrame, double in, double out, double volume):
    _filename (filename),
    _startFrame(startFrame),
    _in(in),
    _out(out),
    _volume(volume),
    _isPlaying (false)
{
    _playback.setMedia (QUrl::fromLocalFile(filename));
}



SoundEffect::SoundEffect(const SoundEffect &sfx) :
    _filename(sfx._filename),
    _startFrame(sfx._startFrame),
    _in(sfx._in),
    _out(sfx._out),
    _volume(sfx._volume),
    _isPlaying(false)
{
    if (_filename.length() > 0) {
        _playback.setMedia (QUrl::fromLocalFile(_filename));
    }
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
        if (_filename.length() > 0) {
            _playback.setMedia (QUrl::fromLocalFile(_filename));
        }
    }
    return *this;
}

bool SoundEffect::operator== (const SoundEffect& rhs) const
{
    return (_filename == rhs._filename &&
            _startFrame == rhs._startFrame &&
            _in == rhs._in &&
            _out == rhs._out);
}

bool SoundEffect::operator< (const SoundEffect& rhs) const
{
    if (_startFrame == rhs._startFrame) {
        if (_filename == rhs._filename) {
            if (_in == rhs._in) {
                return _out < rhs._out;
            } else {
                return _in < rhs._in;
            }
        } else {
            return _filename < rhs._filename;
        }
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



void SoundEffect::play () const
{
    if (_out > _in) {
        _playback.stop();
        _isPlaying = true;
        _playback.setPosition(1000 * _in);
        _playback.play();
        QTimer::singleShot (int(1000*(_out-_in)), this, SLOT(stop()));
    }
}


void SoundEffect::playFrom (double t) const
{
    if (_out > _in) {
        _isPlaying = true;
        _playback.setPosition(1000 * (t+_in));
        _playback.play();
        QTimer::singleShot (int(1000*(_out-_in)), this, SLOT(stop()));
    }

}


void SoundEffect::stop () const
{
    if (_isPlaying) {
        _isPlaying = false;
        _playback.stop();
    }
}

void SoundEffect::load (const QJsonObject &json)
{
    _filename = json["filename"].toString();
    _startFrame = json["startFrame"].toInt();
    _in = json["in"].toDouble();
    _out = json["out"].toDouble();

    if (_filename.length() > 0) {
        _playback.setMedia (QUrl::fromLocalFile(_filename));
        _out = double(_playback.duration()) / 1000.0;
    }
}

void SoundEffect::save (QJsonObject &json) const
{
    json["filename"] = _filename;
    json["startFrame"] = _startFrame;
    json["in"] = _in;
    json["out"] = _out;
}
