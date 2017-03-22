#include "waveform.h"

#include "utils.h"
#include <iostream>
#include <QMouseEvent>
#include "settings.h"
#include "settingsdialog.h"

Waveform::Waveform(QWidget *parent) :
    QGraphicsView (parent),
    _preTitlePixels (0),
    _titlePixels (0),
    _moviePixels (0),
    _creditsPixels (0),
    _playheadPixels (0),
    _draggingSelectionRegion (false)
{
    QGraphicsView::setScene(&_scene);
    this->setMouseTracking(true);
    reset();


    _selectionRegion->hide();
    _draggingSelectionRegion->hide();
    _cursorLine->hide();
    _playheadLine->hide();
}

void Waveform::reset()
{
    _scene.clear();
    _scene.setSceneRect(0,0,this->width(), this->height());

    _preTitleSelectionRegion = _scene.addRect(0,0,0,this->height());
    _titleSelectionRegion = _scene.addRect(0,0,0,this->height());
    _mainSelectionRegion = _scene.addRect(0,0,this->width(),this->height());
    _creditsSelectionRegion = _scene.addRect(0,0,0,this->height());

    _selectionRegion = new QGraphicsItemGroup;
    _selectionRegion->addToGroup(_preTitleSelectionRegion);
    _selectionRegion->addToGroup(_titleSelectionRegion);
    _selectionRegion->addToGroup(_mainSelectionRegion);
    _selectionRegion->addToGroup(_creditsSelectionRegion);
    _scene.addItem(_selectionRegion);

    _draggingSelectionRegion = new QGraphicsItemGroup;
    _draggingPreTitleSelectionRegion = _scene.addRect(this->rect());
    _draggingTitleSelectionRegion = _scene.addRect(this->rect());
    _draggingMainSelectionRegion = _scene.addRect(this->rect());
    _draggingCreditsSelectionRegion = _scene.addRect(this->rect());
    _draggingSelectionRegion->addToGroup(_draggingPreTitleSelectionRegion);
    _draggingSelectionRegion->addToGroup(_draggingTitleSelectionRegion);
    _draggingSelectionRegion->addToGroup(_draggingMainSelectionRegion);
    _draggingSelectionRegion->addToGroup(_draggingCreditsSelectionRegion);
    _scene.addItem(_draggingSelectionRegion);
    _draggingSelectionRegion->hide();

    QPen cursorPen (QColor(0,0,0,100));
    _cursorLine = _scene.addLine (0,0,0,this->height(), cursorPen);
    _cursorLine->setZValue(11);

    QPen playheadPen (QColor(0,0,0,200));
    _playheadLine = _scene.addLine (0,0,0,this->height(), playheadPen);
    _playheadLine->setZValue(10);

    _scene.setSceneRect(0,0,this->width(), this->height());
    this->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    _totalLength = 0;
    _selectionStart = 0;
    _selectionLength = 0;
    _playheadPosition = 0;
    _maxValue = 0;
}

void Waveform::setDuration (qint64 millis)
{
    _totalLength = millis;
}

void Waveform::addBuffer (const QAudioBuffer &buffer)
{
    if (_totalLength == 0) {
        // Throw an error here, you have to set the length
        // before adding buffers.
    }

    if (buffer.format().codec() != "audio/pcm") {
        // Nothing to do with this buffer, it's not audio data
        return;
    }

    // Start processing chunks:
    if (!isPCMS16LE(buffer.format())) {
        // Someday throw here
        std::cerr << "This is not 16-bit PCM little endian" << std::endl;
        return;
    }

    const quint16 *data = buffer.constData<quint16>();

    qint64 startMicroseconds = buffer.startTime();
    qreal microsecondsPerPixel = 1000.0 * (qreal)_totalLength / (qreal)this->width();
    int sampleRate = buffer.format().sampleRate();
    qreal microsecondsPerSample = 1000000.0 / sampleRate;
    qint32 startPixel = startMicroseconds / microsecondsPerPixel;
    quint32 pixel = 0;

    QVector<qreal> maxValues;
    qreal max = 0.0;

    // We don't really care if the audio is multi-channel or not: it's just getting displayed as
    // the peak of whichever channel is highest
    int numberOfSamplesInThisBuffer = buffer.sampleCount();
    for (int sample = 0; sample < numberOfSamplesInThisBuffer; sample++) {
        qreal v = fabs(pcmToReal(data[sample]));
        max = std::max(max, v);
        if ((sample+1) * microsecondsPerSample > (pixel+1)*microsecondsPerPixel) {
            maxValues.append(max);
            max = 0;
            pixel++;
        }
    }
    // In cases where this buffer isn't an even number of pixels wide, we cheat and just tack on one
    // extra pixel, which looks better than one missing pixel.
    maxValues.append(max);

    for (int pixelOffset = 0; pixelOffset < maxValues.length(); pixelOffset++) {
        qreal pixelValue = maxValues[pixelOffset];
        // Draw a line on the graphics view at the right position
        _scene.addLine(QLineF(pixelOffset+startPixel, this->height(), pixelOffset+startPixel,
                              this->height() - pixelValue * this->height()),
                       QPen(Qt::green, 1));
    }

    // Make sure the various UI objects are showing now...
    _selectionRegion->show();
    _cursorLine->show();
    _playheadLine->show();
}

void Waveform::setSelectionStart (qint64 millis)
{
    _selectionStart = ((double) millis / (double)_totalLength) * this->width();
    UpdateSelectionRectangles();
}

void Waveform::setSelectionLength (qint64 millis)
{
    _selectionLength = ((double) millis / (double)_totalLength) * this->width();
    UpdateSelectionRectangles();
}

void Waveform::setPlayheadPosition (qint64 millis)
{
    _playheadPosition = ((double) millis / (double)_totalLength) * this->width();
    _playheadLine->setX (_playheadPosition);
}

qint64 Waveform::getPlayheadPosition () const
{
    return (qint64)(((double)_playheadPosition/(double)this->width()) * _totalLength);
}

qint64 Waveform::getSelectionStart () const
{
    return (qint64)(((double)_selectionStart/(double)this->width()) * _totalLength);
}

qint64 Waveform::getSelectionLength () const
{
    return _totalLength;
}


void Waveform::UpdateSelectionRectangles ()
{
    if (_selectionLength <= 0) {
        return;
    }
    Settings settings;
    // We actually have four different rectangles to draw
    // for the selection, representing the durations for
    // the pre-title, title, movie, and credits regions
    double preTitleDuration = settings.Get("settings/preTitleScreenDuration").toDouble();
    double titleDuration = settings.Get("settings/titleScreenDuration").toDouble();
    double creditsDuration = settings.Get("settings/creditsDuration").toDouble();

    // Convert all these numbers to pixels:
    _preTitlePixels = preTitleDuration/(_totalLength/1000.0) * this->width();
    _titlePixels = titleDuration/(_totalLength/1000.0) * this->width();
    _creditsPixels = creditsDuration/(_totalLength/1000.0) * this->width();
    _moviePixels = _selectionLength - _preTitlePixels - _titlePixels - _creditsPixels;

    QPen ptPen (QColor(50,50,50,200));
    QBrush ptBrush (QColor(50,50,50,60));

    QPen tPen (QColor(30,30,30,200));
    QBrush tBrush (QColor(30,30,30,60));

    QPen mfPen (QColor(0,0,255,200));
    QBrush mfBrush (QColor(0,0,255,60));

    QPen cPen (QColor(30,30,30,200));
    QBrush cBrush (QColor(30,30,30,60));

    qint64 startPixels = _selectionStart;
    _preTitleSelectionRegion->setPen (ptPen);
    _preTitleSelectionRegion->setBrush(ptBrush);
    _preTitleSelectionRegion->setRect(startPixels,0,_preTitlePixels,this->height());
    startPixels += _preTitlePixels;

    _titleSelectionRegion->setPen (tPen);
    _titleSelectionRegion->setBrush(tBrush);
    _titleSelectionRegion->setRect(startPixels,0,_titlePixels,this->height());
    startPixels += _titlePixels;

    _mainSelectionRegion->setPen (mfPen);
    _mainSelectionRegion->setBrush(mfBrush);
    _mainSelectionRegion->setRect(startPixels,0,_moviePixels,this->height());
    startPixels += _moviePixels;

    _creditsSelectionRegion->setPen (cPen);
    _creditsSelectionRegion->setBrush(cBrush);
    _creditsSelectionRegion->setRect(startPixels,0,_creditsPixels,this->height());

    _selectionRegion->setZValue(1);
    _mainSelectionRegion->setZValue(2);

    // Now do the same for the selection regions, but make them
    // darker
    QPen dptPen (QColor(50,50,50,255));
    QBrush dptBrush (QColor(50,50,50,90));

    QPen dtPen (QColor(30,30,30,255));
    QBrush dtBrush (QColor(30,30,30,90));

    QPen dmfPen (QColor(0,0,255,255));
    QBrush dmfBrush (QColor(0,0,255,90));

    QPen dcPen (QColor(30,30,30,255));
    QBrush dcBrush (QColor(30,30,30,90));

    startPixels = _selectionStart;
    _draggingPreTitleSelectionRegion->setPen (dptPen);
    _draggingPreTitleSelectionRegion->setBrush(dptBrush);
    _draggingPreTitleSelectionRegion->setRect(startPixels,0,_preTitlePixels,this->height());
    startPixels += _preTitlePixels;

    _draggingTitleSelectionRegion->setPen (dtPen);
    _draggingTitleSelectionRegion->setBrush(dtBrush);
    _draggingTitleSelectionRegion->setRect(startPixels,0,_titlePixels,this->height());
    startPixels += _titlePixels;

    _draggingMainSelectionRegion->setPen (dmfPen);
    _draggingMainSelectionRegion->setBrush(dmfBrush);
    _draggingMainSelectionRegion->setRect(startPixels,0,_moviePixels,this->height());
    startPixels += _moviePixels;

    _draggingCreditsSelectionRegion->setPen (dcPen);
    _draggingCreditsSelectionRegion->setBrush(dcBrush);
    _draggingCreditsSelectionRegion->setRect(startPixels,0,_creditsPixels,this->height());

    _draggingSelectionRegion->setZValue(1);
    _draggingMainSelectionRegion->setZValue(2);
}

void Waveform::mousePressEvent(QMouseEvent *event)
{
    _dragStartTime.restart();
    _dragStartX = event->pos().x();
    if (event->x() >= _selectionStart &&
        event->x() <= _selectionStart + _selectionLength) {
        // The mouse was pressed within the selection region
        _currentlyDraggingSelection = true;

        _draggingPreTitleSelectionRegion->setX(_preTitleSelectionRegion->x());
        _draggingTitleSelectionRegion->setX(_titleSelectionRegion->x());
        _draggingMainSelectionRegion->setX(_mainSelectionRegion->x());
        _draggingCreditsSelectionRegion->setX(_creditsSelectionRegion->x());

        _draggingSelectionRegion->show();
        _selectionRegion->hide();
        _cursorLine->hide();
    }
    QGraphicsView::mousePressEvent (event);
}

void Waveform::mouseReleaseEvent(QMouseEvent *event)
{
    if (abs(_dragStartX - event->x()) <= 1) {
        // Maybe this was really a click... how long was the button down?
        if (_dragStartTime.elapsed() < 500) {
            // Move the playhead to the cursor
            _playheadLine->setX(event->x());
            _playheadPosition = event->x();

            double percentage = (double)_playheadPosition / (double)this->width();
            qint64 millis = percentage * _totalLength;
            emit playheadManuallyChanged (millis);
        }
    } else if (_currentlyDraggingSelection) {
        _selectionStart = _draggingPreTitleSelectionRegion->x();
        _preTitleSelectionRegion->setX(_draggingPreTitleSelectionRegion->x());
        _titleSelectionRegion->setX(_draggingTitleSelectionRegion->x());
        _mainSelectionRegion->setX(_draggingMainSelectionRegion->x());
        _creditsSelectionRegion->setX(_draggingCreditsSelectionRegion->x());

        _draggingSelectionRegion->hide();
        _selectionRegion->show();
    }
    _currentlyDraggingSelection = false;
    _cursorLine->show();
    QGraphicsView::mouseReleaseEvent (event);
}

void Waveform::mouseMoveEvent(QMouseEvent *event)
{
    _cursorLine->setX (event->x());
    QGraphicsView::mouseMoveEvent (event);
    if (_currentlyDraggingSelection) {
        int actualX = event->pos().x();

        // Calculate all the distances:
        int dragDistance = actualX - _dragStartX;
        int movieStartPixel = _selectionStart + _preTitlePixels + _titlePixels;
        //int titleStartPixel = _selectionStart + _preTitlePixels;
        int endPixel = _selectionStart + _selectionLength;
        //int creditsStartPixel = endPixel - _creditsPixels;
        int d[5] = {INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX};
        d[0] = movieStartPixel + dragDistance - _playheadPixels; // Movie start to playhead
        d[1] = _selectionStart + dragDistance - _playheadPixels; // Whole thing to playhead
        d[2] = _selectionStart + dragDistance; // Whole thing to t=0
        d[3] = movieStartPixel + dragDistance; // Movie start to t=0
        d[4] = endPixel + dragDistance - this->width(); // Whole thing to the end of the music

        int snapOffset = 0;
        for (int snap = 0; snap < 5; snap++) {
            if (abs(d[snap]) <= SNAP_DISTANCE) {
                // This is our snap
                snapOffset = d[snap];
                break;
            }
        }

        // Reset the dragging version of the selection boxes:
        _draggingPreTitleSelectionRegion->setRect(_preTitleSelectionRegion->rect());
        _draggingTitleSelectionRegion->setRect(_titleSelectionRegion->rect());
        _draggingMainSelectionRegion->setRect(_mainSelectionRegion->rect());
        _draggingCreditsSelectionRegion->setRect(_creditsSelectionRegion->rect());

        // Now shift them into position:
        _draggingPreTitleSelectionRegion->setX(_preTitleSelectionRegion->x()+dragDistance-snapOffset);
        _draggingTitleSelectionRegion->setX(_titleSelectionRegion->x()+dragDistance-snapOffset);
        _draggingMainSelectionRegion->setX(_mainSelectionRegion->x()+dragDistance-snapOffset);
        _draggingCreditsSelectionRegion->setX(_creditsSelectionRegion->x()+dragDistance-snapOffset);
    } else {
        _cursorLine->show();
    }
}

void Waveform::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent (event);
}

void Waveform::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent (event);
}
