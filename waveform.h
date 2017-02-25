#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QGraphicsView>
#include <QAudioBuffer>

/**
 * @brief The Waveform class displays an audio waveform and allows interaction
 * with it to set a cursor position (for a playhead) and a selection (to grab
 * a subset of the whole audio file).
 */
class Waveform : public QGraphicsView
{
public:
    Waveform(QWidget *parent = NULL);

    void reset();

    void setDuration (qint64 millis);

    void addBuffer (const QAudioBuffer &buffer);

    void setSelectionStart (qint64 millis);

    void setSelectionLength (qint64 millis);

    void setCursorPosition (qint64 millis);

    qint64 getCursorPosition () const;

    qint64 getSelectionStart () const;

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragLeaveEvent(QDragLeaveEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

private:
    QGraphicsScene _scene;
    qint64 _totalLength;
    qint64 _selectionStart;
    qint64 _selectionLength;
    qint64 _cursorPosition;
    qreal _maxValue;
};

#endif // WAVEFORM_H
