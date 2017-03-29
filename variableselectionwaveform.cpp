#include "variableselectionwaveform.h"
#include <QMouseEvent>

VariableSelectionWaveform::VariableSelectionWaveform(QWidget *parent) :
    Waveform(parent),
    _currentlyDragging (false)
{
    connect (this, &Waveform::selectionRegionChanged, this, &VariableSelectionWaveform::onSelectionRegionChanged);
}

void VariableSelectionWaveform::mousePressEvent(QMouseEvent *event)
{
    _selectionStart = event->pos().x();
    _selectionLength = 1;
    _dragStartTime.start();
    _dragStartX = _selectionStart;
    _currentlyDragging = true;
    emit (selectionRegionChanged(_selectionStart, _selectionLength));
    Waveform::mousePressEvent(event);
}

void VariableSelectionWaveform::mouseReleaseEvent(QMouseEvent *event)
{
    _currentlyDragging = false;

    // Make our selection always positive...
    if (_selectionLength < 0) {
        _selectionStart += _selectionLength;
        _selectionLength = abs(_selectionLength);
    }
    emit (selectionRegionChanged(_selectionStart, _selectionLength));
    setPlayheadPosition(pixelsToMillis(_selectionStart));
    emit (playheadManuallyChanged(pixelsToMillis(_selectionStart)));
    Waveform::mouseReleaseEvent(event);
}

void VariableSelectionWaveform::mouseMoveEvent(QMouseEvent *event)
{
    if (_currentlyDragging) {
        int currentX = event->pos().x();
        _selectionLength = currentX - _dragStartX;
        emit (selectionRegionChanged(_selectionStart, _selectionLength));
    }
    Waveform::mouseMoveEvent(event);
}


void VariableSelectionWaveform::onSelectionRegionChanged (qint64 start, qint64 length)
{
    int rStart, rWidth;
    if (length >= 0) {
        rStart = start;
        rWidth = length;
    } else {
        rStart = start + length;
        rWidth = abs(length);
    }
    QRectF r (rStart, 0, rWidth, _scene.height());
    if (_scene.items().contains(_selectionRegion)) {
        _selectionRegion->setRect (r);
    } else {
        QPen pen (Qt::black);
        QBrush brush (QColor(0,0,0,70));
        _selectionRegion = _scene.addRect (r, pen, brush);
        _selectionRegion->setZValue(100);
    }
}