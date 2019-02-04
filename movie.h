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
#include <QProcess>

#include "avcodecwrapper.h"


class Movie : public QObject
{
    Q_OBJECT

public:
    Movie(const QString &name = "", bool allowModifications = true);

    ~Movie ();

    void setName (const QString &name);

    QString getName () const;

    QString getEncodingFilename() const;

    QString getEncodingTitle() const;

    QString getEncodingCredits() const;

    QString getSaveFilename () const;

    void setCamera (QCamera *camera);

    void addFrame (bool rotate180 = false);

    void importFrame (const QString &filename);

    int getNumberOfFrames () const;

    void deleteLastFrame ();

    QString getMostRecentFrame () const;

    void setStillFrame (int frameNumber, QLabel *video);

    void play (int startFrame, QLabel *video);

    void playSoundEffect (const SoundEffect &sfx, QLabel *video);

    void stop ();

    void addBackgroundMusic (const SoundEffect &backgroundMusic);

    SoundEffect getBackgroundMusic () const;

    void addSoundEffect (const SoundEffect &soundEffect);

    QList<SoundEffect> getSoundEffects () const;

    SoundEffect getSoundEffect (int frame) const;

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

    void encodeToFile (const QString &filename, const QString &title, const QString &credits);

signals:

    void frameChanged (int newFrame);


protected slots:

    void nextFrame ();

    void readyForCaptureChanged (bool ready);

    void imageSaved (int id, const QString &fileName);

protected:

    QString getBaseFilename () const;

    QString getImageFilename (int frame) const;

    void CreatePreTitle(avcodecWrapper &encoder) const;
    void CreateTitle(avcodecWrapper &encoder, const QString &title) const;
    void CreateCredits(avcodecWrapper &encoder, const QString &credits) const;

private:

    QString _name;
    qint32 _numberOfFrames;
    qint32 _framesPerSecond;
    QMap<int,SoundEffect> _soundEffects;
    SoundEffect _backgroundMusic;
    QString _encodingFilename;
    QString _encodingTitle;
    QString _encodingCredits;
    bool _allowModifications;

    QCamera *_camera;
    QImageEncoderSettings _encoderSettings;
    std::unique_ptr<QCameraImageCapture> _imageCapture;
    mutable QStringList _encodingTempFiles;
    QString _fileForRotation;

    // For playback
    bool _currentlyPlaying;
    qint32 _currentFrame;
    QTimer _playbackTimer;
    QLabel *_frameDestination;
    qint32 _playFrameCounter;
    qint32 _skippedFrameCounter;
    qint32 _computerSpeedAdjust;
    QTime _playStartTime;
    bool _mute;

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

    class CaptureFailedException : public QException
    {
    public:
        CaptureFailedException(QString message = "") {_message = message;}
        void raise() const { throw *this; }
        CaptureFailedException *clone() const {return new CaptureFailedException(*this); }
        QString message() const {return _message;}
    private:
        QString _message;
    };

    class EncodingFailedException : public QException
    {
    public:
        EncodingFailedException(QString message = "") {_message = message;}
        void raise() const { throw *this; }
        EncodingFailedException *clone() const {return new EncodingFailedException(*this); }
        QString message() const {return _message;}
    private:
        QString _message;
    };

};

#endif // MOVIE_H
