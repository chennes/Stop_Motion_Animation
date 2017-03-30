#ifndef MULTIREGIONWAVEFORM_H
#define MULTIREGIONWAVEFORM_H

#include "waveform.h"

class MultiRegionWaveform : public Waveform
{
public:
    MultiRegionWaveform(QWidget *parent = nullptr);

    void AddRegion (QString name, qint64 startMillis, qint64 endMillis, QColor color = QColor());

    void ClearRegions ();

    void DoneAddingRegions ();

    virtual void reset ();

    // Events we need to handle
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);

    struct Region {
        qint64 startMillis;
        qint64 endMillis;
        int pixelStart;
        int pixelWidth;
        QColor penColor;
        QColor brushColor;
        QPen pen;
        QBrush brush;
        QGraphicsRectItem *rect;
        QGraphicsTextItem *text;
    };

private:

    std::vector<Region> _regions;
    QGraphicsItemGroup *_group;

    // We are going to manually manage dragging for now:
    const int SNAP_DISTANCE = 10; // pixels
    bool _currentlyDraggingSelection;
    int _dragStartX;
    QTime _dragStartTime;
    bool _locked;
};

#endif // MULTIREGIONWAVEFORM_H
