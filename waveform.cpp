#include "waveform.h"

#include "utils.h"
#include <iostream>

Waveform::Waveform(QWidget *parent) :
    QGraphicsView (parent)
{
    // The scene rectangle is set to the width and height in
    // pixels for now.
    _scene.setSceneRect(0,0,this->width(), this->height());
    QGraphicsView::setScene(&_scene);

    reset();
}

void Waveform::reset()
{
    _scene.clear();
    _scene.setSceneRect(0,0,this->width(), this->height());
    this->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    _totalLength = 0;
    _selectionStart = 0;
    _selectionLength = 0;
    _cursorPosition = 0;
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
}

void Waveform::setSelectionStart (qint64 millis)
{
    _selectionStart = millis;
}

void Waveform::setSelectionLength (qint64 millis)
{
    _selectionLength = millis;
}

void Waveform::setCursorPosition (qint64 millis)
{
    _cursorPosition = millis;
}

qint64 Waveform::getCursorPosition () const
{
    return _cursorPosition;
}

qint64 Waveform::getSelectionStart () const
{
    return _selectionStart;
}

void Waveform::mousePressEvent(QMouseEvent *event)
{

}

void Waveform::mouseReleaseEvent(QMouseEvent *event)
{

}

void Waveform::dragEnterEvent(QDragEnterEvent *event)
{

}

void Waveform::dragLeaveEvent(QDragLeaveEvent *event)
{

}

void Waveform::dragMoveEvent(QDragMoveEvent *event)
{

}

void Waveform::resizeEvent(QResizeEvent *event)
{

}
