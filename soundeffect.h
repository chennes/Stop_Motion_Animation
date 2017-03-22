#ifndef SOUNDEFFECT_H
#define SOUNDEFFECT_H

#include <QObject>
#include <QtMultimedia/QMediaPlayer>

class SoundEffect : public QObject
{
    Q_OBJECT

public:
    SoundEffect(const QString &filename = "");

    SoundEffect(const QString &filename, double start, double in, double out);

    SoundEffect(const SoundEffect &sfx);

    SoundEffect& operator= (const SoundEffect& rhs);

    bool operator== (const SoundEffect& rhs) const;

    bool operator< (const SoundEffect& rhs) const;

    explicit operator bool() const;

    void setStartTime (double t);
    void setInPoint (double t);
    void setOutPoint (double t);
    void setVolume (double v);

    QString getFilename () const;
    double getStartTime () const;
    double getInPoint () const;
    double getOutPoint () const;
    double getVolume () const;

    void play () const;

    /**
     * @brief playFrom cues the playback at *local* time t.
     * @param t The start time in seconds
     *
     * This function moves forward t seconds after the in point and starts
     * playback from there. Its purpose is to allow starting playback in
     * the middle of a sound effect, but care must be taken to account for
     * the start time of this clip. While background music probably starts
     * at t=0, most sound effects will not.
     */
    void playFrom (double t) const;

    void stop () const;

    void load(const QJsonObject &json);

    void save (QJsonObject &json) const;


private:
    QString _filename;
    double _startTime;
    double _in;
    double _out;
    double _volume;

    mutable bool _isPlaying;
    mutable QMediaPlayer _playback;
};

#endif // SOUNDEFFECT_H
