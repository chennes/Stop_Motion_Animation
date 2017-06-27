#include "variableselectionwaveform.h"
#include <QMouseEvent>

VariableSelectionWaveform::VariableSelectionWaveform(QWidget *parent) :
    Waveform(parent),
    _selectionRegion(NULL),
    _currentlyDragging (false)
{
    connect (this, &Waveform::selectionRegionChanged, this, &VariableSelectionWaveform::onSelectionRegionChanged);
}

void VariableSelectionWaveform::reset ()
{
    _selectionRegion = NULL; // Lose the pointer, we don't own the item and it's about to get deleted
    _currentlyDragging = false;
    Waveform::reset();
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
    if (_selectionRegion && _scene.items().contains(_selectionRegion)) {
        _selectionRegion->setRect (r);
    } else {
        QPen pen (Qt::black);
        QBrush brush (QColor(0,0,0,70));
        _selectionRegion = _scene.addRect (r, pen, brush);
        _selectionRegion->setZValue(100);
    }

}

qint64 VariableSelectionWaveform::getSelectionLength () const
{
    // If the selection is super small, assume it wasn't meant to be a selection at all
    if (_selectionLength < 5) {
        int fullWidth = this->width() - _selectionStart;
        return pixelsToMillis(fullWidth);
    } else {
        return pixelsToMillis(_selectionLength);
    }
}
