#ifndef PREVIOUSFRAMEOVERLAYEFFECT_H
#define PREVIOUSFRAMEOVERLAYEFFECT_H

#include <QGraphicsEffect>
#include <QPixmap>
#include <QException>

class PreviousFrameOverlayEffect : public QGraphicsEffect
{
    Q_OBJECT

public:
    PreviousFrameOverlayEffect();

    enum class Mode {
        PREVIOUS,
        BLEND
    };

    void setPreviousFrame (const QString &filename);
    void setMode (Mode newMode);
    Mode getMode () const;

protected:
    virtual void draw(QPainter *painter);

private:

    QString _previousFrameFile;
    bool _frameNeedsUpdate;
    QPixmap _previousFrame;
    Mode _mode;


public:


    class LoadFailedException : public QException
    {
    public:
        LoadFailedException(QString m = "") {_message = m;}
        void raise() const { throw *this; }
        LoadFailedException *clone() const {return new LoadFailedException(*this); }
        QString message() const {return _message;}
    private:
        QString _message;
    };
};

#endif // PREVIOUSFRAMEOVERLAYEFFECT_H
