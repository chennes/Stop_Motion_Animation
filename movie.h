#ifndef MOVIE_H
#define MOVIE_H

#include <QObject>
#include <QException>
#include <memory>

#include <string>
#include <soundeffect.h>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraImageCapture>
#include <QLabel>
#include <QTimer>
#include <QTime>


class Movie : public QObject
{
    Q_OBJECT

public:
    Movie(const QString &name = "");

    ~Movie ();

    void setName (const QString &name);

    QString getName () const;

    void addFrame (QCamera *camera);

    void Movie::importFrame (const QString &filename);

    int getNumberOfFrames () const;

    void deleteLastFrame ();

    void setStillFrame (int frameNumber, QLabel *video);

    void play (int startFrame, QLabel *video);

    void stop ();

    void addBackgroundMusic (const SoundEffect &backgroundMusic);

    SoundEffect getBackgroundMusic () const;

    void addSoundEffect (const SoundEffect &soundEffect);

    QList<SoundEffect> getSoundEffects () const;

    void removeBackgroundMusic();

    void removeSoundEffect (const SoundEffect &soundEffect);

    void removeAllSoundEffects ();

    /**
     * @brief Serialize the local data from this object
     */
    void save () const;

    /**
     * @brief Unserialize the local data from this object
     */
    bool load(const QString &filename);

    void encodeToFile (const QString &filename) const;

signals:

    void frameChanged (int newFrame);


protected slots:

    void nextFrame ();

protected:

    QString getBaseFilename () const;

    QString getSaveFilename () const;

    QString getImageFilename (int frame) const;

private:

    static const int DEFAULT_FPS;

    QString _name;
    qint32 _numberOfFrames;
    qint32 _framesPerSecond;
    QList<SoundEffect> _soundEffects;
    SoundEffect _backgroundMusic;

    QCamera *_camera;
    QImageEncoderSettings _encoderSettings;
    std::unique_ptr<QCameraImageCapture> _imageCapture;

    // For playback
    bool _currentlyPlaying;
    qint32 _currentFrame;
    QTimer _playbackTimer;
    QLabel *_frameDestination;
    qint32 _playFrameCounter;
    QTime _playStartTime;
    QTime _lastFrameTime;
    qint32 _computerSpeedAdjustment;

public:

    class NoChangesNowException : public QException
    {
    public:
        NoChangesNowException(QString m = "") {_message = m;}
        void raise() const { throw *this; }
        NoChangesNowException *clone() const {return new NoChangesNowException(*this); }
        QString message() const {return _message;}
    private:
        QString _message;
    };

    class FailedToSaveException : public QException
    {
    public:
        FailedToSaveException(QString filename = "") {_filename = filename;}
        void raise() const { throw *this; }
        FailedToSaveException *clone() const {return new FailedToSaveException(*this); }
        QString filename() const {return _filename;}
    private:
        QString _filename;
    };

    class ImportFailedException : public QException
    {
    public:
        ImportFailedException() {}
        void raise() const { throw *this; }
        ImportFailedException *clone() const {return new ImportFailedException(*this); }
    };

};

#endif // MOVIE_H
