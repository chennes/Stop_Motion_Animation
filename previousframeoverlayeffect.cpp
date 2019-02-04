#include "previousframeoverlayeffect.h"

#include <QPainter>

#include <iostream>
#include <QFile>
#include "settings.h"

PreviousFrameOverlayEffect::PreviousFrameOverlayEffect() :
    _frameNeedsUpdate (false),
    _mode (Mode::BLEND)
{
}

void PreviousFrameOverlayEffect::setPreviousFrame (const QString &filename)
{
    _previousFrameFile = filename;
    _frameNeedsUpdate = true;
}

void PreviousFrameOverlayEffect::setMode (Mode newMode)
{
    _mode = newMode;
}

PreviousFrameOverlayEffect::Mode PreviousFrameOverlayEffect::getMode () const
{
    return _mode;
}

void PreviousFrameOverlayEffect::draw(QPainter *painter)
{
    if (_frameNeedsUpdate) {
        bool loaded = false;
        try {
            loaded = _previousFrame.load (_previousFrameFile);
        } catch (...) {
            loaded = false;
        }

        if (!loaded) {
            std::cerr << "Could not load the frame file!" << std::endl;
            std::cerr << _previousFrameFile.toStdString() << std::endl;
        }
        _frameNeedsUpdate = false;
    }
    const QPixmap pixmap = sourcePixmap();
    const QRect rect = pixmap.rect();

    QImage overlaidImage (rect.width(), rect.height(), QImage::Format_ARGB32_Premultiplied);
    QPainter overlaidPainter (&overlaidImage);
    overlaidPainter.setCompositionMode(QPainter::CompositionMode_Source);

    Settings settings;
    int w = settings.Get("settings/imageWidth").toInt();
    int h = settings.Get("settings/imageHeight").toInt();

    if (_mode == Mode::BLEND) {
        overlaidPainter.drawPixmap(0,0,w,h,pixmap);
        overlaidPainter.setCompositionMode(QPainter::CompositionMode_Screen);
    }
    overlaidPainter.drawPixmap(0,0,w,h, _previousFrame);
    overlaidPainter.end();

    painter->drawImage(rect, overlaidImage);
}
