#include "waveform.h"

#include "utils.h"
#include <iostream>
#include <QMouseEvent>
#include "settings.h"
#include "settingsdialog.h"

Waveform::Waveform(QWidget *parent) :
    QGraphicsView (parent),
    _playheadPixels (0)
{
    QGraphicsView::setScene(&_scene);
    this->setMouseTracking(true);

    _scene.clear();
    _scene.setSceneRect(0,0,this->width(), this->height());
    QPen cursorPen (QColor(0,0,0,100));
    _cursorLine = _scene.addLine (0,0,0,this->height(), cursorPen);
    QPen playheadPen (QColor(0,0,0,200));
    _playheadLine = _scene.addLine (0,0,0,this->height(), playheadPen);
    _cursorLine->hide();
    _playheadLine->hide();

    this->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);

    _totalLength = 0;
    _selectionStart = 0;
    _selectionLength = 0;
    _playheadPosition = 0;
    _maxValue = 0;
}

void Waveform::reset()
{
    _scene.clear();
    _scene.setSceneRect(0,0,this->width(), this->height());
    QPen cursorPen (QColor(0,0,0,100));
    _cursorLine = _scene.addLine (0,0,0,this->height(), cursorPen);
    _cursorLine->setZValue(11);

    QPen playheadPen (QColor(0,0,0,200));
    _playheadLine = _scene.addLine (0,0,0,this->height(), playheadPen);
    _playheadLine->setZValue(10);

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
    _cursorLine->show();
    _playheadLine->show();
}

void Waveform::setSelectionStart (qint64 millis)
{
    _selectionStart = millisToPixels(millis);
    emit selectionRegionChanged(_selectionStart, _selectionLength);
}

void Waveform::setSelectionLength (qint64 millis)
{
    _selectionLength = millisToPixels(millis);
    emit selectionRegionChanged(_selectionStart, _selectionLength);
}

void Waveform::setPlayheadPosition (qint64 millis)
{
    _playheadPosition = millisToPixels(millis);
    _playheadLine->setX (_playheadPosition);
}

qint64 Waveform::getDuration () const
{
    return _totalLength;
}

qint64 Waveform::getPlayheadPosition () const
{
    return pixelsToMillis(_playheadPosition);
}

qint64 Waveform::getSelectionStart () const
{
    return pixelsToMillis(_selectionStart);
}

qint64 Waveform::getSelectionLength () const
{
    return _totalLength;
}



void Waveform::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent (event);
}

void Waveform::mouseReleaseEvent(QMouseEvent *event)
{
    qint64 millis = pixelsToMillis(event->x());
    setPlayheadPosition(millis);
    QGraphicsView::mouseReleaseEvent(event);
    emit playheadManuallyChanged (millis);
}

void Waveform::mouseMoveEvent(QMouseEvent *event)
{
    _cursorLine->setX (event->x());
    QGraphicsView::mouseMoveEvent (event);
}



qint64 Waveform::pixelsToMillis(int pixels) const
{
    return (double)pixels / (double)_scene.width() * _totalLength;
}


int Waveform::millisToPixels (qint64 millis) const
{
    return (double) millis / (double) _totalLength * _scene.width();
}
