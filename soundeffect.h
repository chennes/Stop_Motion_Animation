#ifndef SOUNDEFFECT_H
#define SOUNDEFFECT_H

#include <QObject>
#include <QtMultimedia/QMediaPlayer>

class SoundEffect : public QObject
{
    Q_OBJECT

public:
    SoundEffect(const QString &filename = "");

    SoundEffect(const SoundEffect &sfx);

    SoundEffect& operator= (const SoundEffect& rhs);

    bool operator== (const SoundEffect& rhs) const;

    bool operator< (const SoundEffect& rhs) const;

    void setStartTime (float t);
    void setInPoint (float t);
    void setOutPoint (float t);

    float getStartTime () const;
    float getInPoint () const;
    float getOutPoint () const;

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
    void playFrom (float t) const;

    void stop () const;

    void load(const QJsonObject &json);

    void save (QJsonObject &json) const;


private:
    QString _filename;
    double _startTime;
    double _in;
    double _out;

    mutable bool _isPlaying;
    mutable QMediaPlayer _playback;
};

#endif // SOUNDEFFECT_H