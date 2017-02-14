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


class Movie : public QObject
{
    Q_OBJECT

public:
    Movie(const QString &name = "");

    void setName (const QString &name);

    QString getName () const;

    void addFrame (QCamera *camera);

    unsigned int getNumberOfFrames () const;

    void deleteLastFrame ();

    void setStillFrame (unsigned int frameNumber, QLabel *video);

    void setFramesPerSecond (unsigned int fps);

    unsigned int getFramesPerSecond () const;

    void setResolution (int w, int h);

    QSize getResolution () const;

    void setFormat (const QString &extension);

    QString getFormat () const;

    QList<QByteArray> getSupportedFormats () const;

    void play (unsigned int startFrame, QLabel *video);

    void stop ();

    void addBackgroundMusic (std::shared_ptr<SoundEffect> backgroundMusic);

    void addSoundEffect (std::shared_ptr<SoundEffect> soundEffect);

    void removeBackgroundMusic();

    void removeSoundEffect (std::shared_ptr<SoundEffect> soundEffect);

    /**
     * @brief Serialize the local data from this object
     */
    void save () const;

    /**
     * @brief Unserialize the local data from this object
     */
    void load ();

    void encodeToFile (const QString &filename) const;

signals:

    void frameChanged (unsigned int newFrame);


protected slots:

    void nextFrame ();

protected:

    QString getBaseFilename () const;

    QString getSaveFilename () const;

    QString getImageFilename (unsigned int frame) const;

private:

    static const unsigned int DEFAULT_FPS;

    QString _name;
    unsigned int _numberOfFrames;
    unsigned int _framesPerSecond;
    std::vector<std::shared_ptr<SoundEffect>> _soundEffects;
    std::shared_ptr<SoundEffect> _backgroundMusic;

    QCamera *_camera;
    QImageEncoderSettings _encoderSettings;
    std::unique_ptr<QCameraImageCapture> _imageCapture;

    // For playback
    bool _currentlyPlaying;
    unsigned int _currentFrame;
    QTimer _playbackTimer;
    QLabel *_frameDestination;

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

};

#endif // MOVIE_H
