#ifndef VARIABLESELECTIONWAVEFORM_H
#define VARIABLESELECTIONWAVEFORM_H

#include "waveform.h"

class VariableSelectionWaveform : public Waveform
{
public:
    VariableSelectionWaveform(QWidget *parent = nullptr);

    virtual qint64 getSelectionLength () const;
    virtual void reset ();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);


protected slots:
    void onSelectionRegionChanged (qint64 start, qint64 length);


private:
    QGraphicsRectItem *_selectionRegion;
    bool _currentlyDragging;
    qint64 _dragStartX;
    QTime _dragStartTime;
};

#endif // VARIABLESELECTIONWAVEFORM_H
