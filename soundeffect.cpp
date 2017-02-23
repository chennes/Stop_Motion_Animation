#include "soundeffect.h"

#include <QTimer>
#include <QJsonObject>

SoundEffect::SoundEffect(const QString &filename):
    _filename (filename),
    _startTime(0.0),
    _in(0.0),
    _isPlaying (false)
{
    if (_filename.length() > 0) {
        _playback.setMedia (QUrl::fromLocalFile(filename));
        _out = float(_playback.duration()) / 1000.0;
    }
}



SoundEffect::SoundEffect(const SoundEffect &sfx) :
    _filename(sfx._filename),
    _startTime(sfx._startTime),
    _in(sfx._in),
    _out(sfx._out),
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
        _startTime = rhs._startTime;
        _in = rhs._in;
        _out = rhs._out;
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
            _startTime == rhs._startTime &&
            _in == rhs._in &&
            _out == rhs._out);
}

bool SoundEffect::operator< (const SoundEffect& rhs) const
{
    if (_startTime == rhs._startTime) {
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
        return _startTime < rhs._startTime;
    }
}

SoundEffect::operator bool() const
{
    return (_filename.length()>0);
}

void SoundEffect::setStartTime (float t)
{
    _startTime = t;
}


void SoundEffect::setInPoint (float t)
{
    _in = t;
}


void SoundEffect::setOutPoint (float t)
{
    _out = t;
}



float SoundEffect::getStartTime () const
{
    return _startTime;
}


float SoundEffect::getInPoint () const
{
    return _in;
}


float SoundEffect::getOutPoint () const
{
    return _out;
}



void SoundEffect::play () const
{
    if (_out > _in) {
        _isPlaying = true;
        _playback.setPosition(1000 * _in);
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
    _startTime = json["startTime"].toDouble();
    _in = json["in"].toDouble();
    _out = json["out"].toDouble();

    if (_filename.length() > 0) {
        _playback.setMedia (QUrl::fromLocalFile(_filename));
        _out = float(_playback.duration()) / 1000.0;
    }
}

void SoundEffect::save (QJsonObject &json) const
{
    json["filename"] = _filename;
    json["startTime"] = _startTime;
    json["in"] = _in;
    json["out"] = _out;
}
