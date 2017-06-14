#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QGraphicsView>
#include <QAudioBuffer>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QTime>


/**
 * @brief The Waveform class displays an audio waveform and allows interaction
 * with it to set a cursor position (for a playhead) and a selection (to grab
 * a subset of the whole audio file).
 */
class Waveform : public QGraphicsView
{
    Q_OBJECT

public:
    Waveform(QWidget *parent = NULL);

    virtual void reset();

    void setDuration (qint64 millis);

    void addBuffer (const QAudioBuffer &buffer);

    void setSelectionStart (qint64 millis);

    void setSelectionLength (qint64 millis);

    void setPlayheadPosition (qint64 millis);

    qint64 getDuration () const;

    qint64 getPlayheadPosition () const;

    qint64 getSelectionStart () const;

    virtual qint64 getSelectionLength () const;

signals:

    void playheadManuallyChanged (qint64 millis);

    void selectionRegionChanged (qint64 start, qint64 length);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    //virtual void paintEvent(QPaintEvent *event);

    qint64 pixelsToMillis(int pixels) const;
    int millisToPixels (qint64 millis) const;

protected:
    QGraphicsScene _scene;
    qint64 _totalLength;
    qint64 _selectionStart;
    qint64 _selectionLength;
    qint64 _playheadPosition;
    qreal _maxValue;

    // Elements in the scene that we need to change over time:
    QGraphicsLineItem *_cursorLine;
    QGraphicsLineItem *_playheadLine;

    qint64 _playheadPixels;
};

#endif // WAVEFORM_H
