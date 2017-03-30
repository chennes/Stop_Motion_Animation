#include "MultiRegionWaveform.h"

#include "utils.h"
#include <iostream>
#include <QMouseEvent>
#include "settings.h"
#include "settingsdialog.h"

MultiRegionWaveform::MultiRegionWaveform(QWidget *parent) :
    Waveform (parent),
    _group (nullptr),
    _currentlyDraggingSelection (false)
{
}

void MultiRegionWaveform::AddRegion (QString name, qint64 startMillis, qint64 endMillis, QColor color)
{
    _regions.push_back(Region());
    auto &&r = _regions.back();
    r.startMillis = startMillis;
    r.endMillis = endMillis;
    if (_totalLength > 0) {
        r.pixelWidth = _scene.width() * (double(endMillis-startMillis)/_totalLength);
        r.pixelStart = _selectionStart + _scene.width() * (double(startMillis)/_totalLength);
    }
    r.penColor = color;
    r.brushColor = QColor (color.red(), color.green(), color.blue(), 0.3 * color.alpha());
    r.pen = QPen (r.penColor);
    r.brush = QBrush (r.brushColor);
    r.rect = _scene.addRect(r.pixelStart,0,r.pixelWidth,this->height(),r.pen, r.brush);
    r.text = _scene.addText(name);
    r.text->setDefaultTextColor (r.penColor);
    r.text->setX(r.pixelStart);

    QList<QGraphicsItem *> groupedItems;
    groupedItems.append(r.text);
    groupedItems.append(r.rect);

    if (_group == nullptr) {
        _group = _scene.createItemGroup(groupedItems);
    } else {
        _group->addToGroup(r.rect);
        _group->addToGroup(r.text);
    }

    // Every time we add a new region, check to see if the new text object intersects any
    // of the others, and if it does, stagger it down
    for (int regionId = 0; regionId < _regions.size()-1; regionId++) {
        Region &region = _regions[regionId];
        QGraphicsTextItem *t = region.text;
        QRectF checkRect = t->mapRectFromScene(t->boundingRect());
        QRectF newTextRect = r.text->mapRectFromScene(r.text->boundingRect());
        if (newTextRect.intersects(checkRect)) {
            r.text->setY(r.text->y()+r.text->boundingRect().height()+1);
        }
    }
    qint64 currentMillis = getSelectionLength();
    setSelectionLength(std::max (currentMillis, endMillis));
}

void MultiRegionWaveform::ClearRegions ()
{
    for (auto &&r:_regions) {
        _group->removeFromGroup(r.text);
        _group->removeFromGroup(r.rect);
        _scene.removeItem(r.text);
        _scene.removeItem(r.rect);
    }
    _regions.clear();
    _group = NULL;
}

void MultiRegionWaveform::reset()
{
    ClearRegions();
    Waveform::reset();
}

void MultiRegionWaveform::mousePressEvent(QMouseEvent *event)
{
    _dragStartTime.restart();
    _dragStartX = event->pos().x();
    if (event->x() >= _selectionStart &&
        event->x() <= _selectionStart + _selectionLength) {
        // The mouse was pressed within the selection region
        _currentlyDraggingSelection = true;
        _cursorLine->hide();
    }

    // Skip over Waveform, it will want to allow arbitary selection regions,
    // go straight to its parent...
    QGraphicsView::mousePressEvent (event);
}

void MultiRegionWaveform::mouseReleaseEvent(QMouseEvent *event)
{
    if (abs(_dragStartX - event->x()) <= 1) {
        // Maybe this was really a click... how long was the button down?
        if (_dragStartTime.elapsed() < 500) {
            // Let our parent handle it if it wants to
            Waveform::mouseReleaseEvent(event);
        }
    } else if (_currentlyDraggingSelection) {
        int actualX = event->pos().x();
        int dragDistance = actualX - _dragStartX;
        _selectionStart += dragDistance;
        _group->setX(_selectionStart);
        QGraphicsView::mouseReleaseEvent (event);
    } else {
        Waveform::mouseReleaseEvent(event);
    }
    _currentlyDraggingSelection = false;
    _cursorLine->show();
}

void MultiRegionWaveform::mouseMoveEvent(QMouseEvent *event)
{
    _cursorLine->setX (event->x());
    QGraphicsView::mouseMoveEvent (event);
    if (_currentlyDraggingSelection) {
        int actualX = event->pos().x();
        int dragDistance = actualX - _dragStartX;
        _group->setX(_selectionStart + dragDistance);
    } else {
        _cursorLine->show();
    }
}
