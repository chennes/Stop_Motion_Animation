#include "soundeffect.h"

#include <QTimer>

SoundEffect::SoundEffect(const std::string &filename):
    _filename (filename),
    _startTime(0.0),
    _in(0.0),
    _isPlaying (false)
{
    _playback.setMedia (QUrl::fromLocalFile(QString::fromStdString(filename)));
    _out = float(_playback.duration()) / 1000.0;
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


std::ostream& operator<<(std::ostream& os, const SoundEffect& se)
{
    os << se._filename << "\n"
       << se._startTime << "\n"
       << se._in << "\n"
       << se._out << "\n";
    return os;
}

std::istream& operator>>(std::istream& is, SoundEffect& se)
{
    is >> se._filename
       >> se._startTime
       >> se._in
       >> se._out;
    return is;
}
