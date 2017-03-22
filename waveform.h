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

    void reset();

    void setDuration (qint64 millis);

    void addBuffer (const QAudioBuffer &buffer);

    void setSelectionStart (qint64 millis);

    void setSelectionLength (qint64 millis);

    void setPlayheadPosition (qint64 millis);

    qint64 getPlayheadPosition () const;

    qint64 getSelectionStart () const;

    qint64 getSelectionLength () const;

signals:

    void playheadManuallyChanged (qint64 millis);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void paintEvent(QPaintEvent *event);

private:
    void UpdateSelectionRectangles ();

private:
    QGraphicsScene _scene;
    qint64 _totalLength;
    qint64 _selectionStart;
    qint64 _selectionLength;
    qint64 _playheadPosition;
    qreal _maxValue;

    //Track some extra distances to make the code clearer
    qint64 _preTitlePixels;
    qint64 _titlePixels;
    qint64 _moviePixels;
    qint64 _creditsPixels;
    qint64 _playheadPixels;

    // We are going to manually manage dragging for now:
    const int SNAP_DISTANCE = 10; // pixels
    bool _currentlyDraggingSelection;
    int _dragStartX;
    QTime _dragStartTime;

    // Elements in the scene that we need to change over time:
    QGraphicsLineItem *_cursorLine;
    QGraphicsLineItem *_playheadLine;
    QGraphicsItemGroup *_selectionRegion;
    QGraphicsRectItem *_preTitleSelectionRegion;
    QGraphicsRectItem *_titleSelectionRegion;
    QGraphicsRectItem *_mainSelectionRegion;
    QGraphicsRectItem *_creditsSelectionRegion;

    // Same as above, but for use when the user is dragging them
    QGraphicsItemGroup *_draggingSelectionRegion;
    QGraphicsRectItem *_draggingPreTitleSelectionRegion;
    QGraphicsRectItem *_draggingTitleSelectionRegion;
    QGraphicsRectItem *_draggingMainSelectionRegion;
    QGraphicsRectItem *_draggingCreditsSelectionRegion;
};

#endif // WAVEFORM_H
