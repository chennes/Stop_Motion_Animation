#include "movieframeslider.h"

#include <QMouseEvent>

MovieFrameSlider::MovieFrameSlider(QWidget *parent) :
    QSlider (parent)
{
}


void MovieFrameSlider::mousePressEvent(QMouseEvent *ev)
{
    // Figure out what frame this corresponds to, then set the value
    double percentageLocation {double(ev->x()) / double(this->width())};
    int frame {int(round(this->minimum() + percentageLocation * (this->maximum() - this->minimum())))};
    setValue (frame);
}


void MovieFrameSlider::mouseMoveEvent(QMouseEvent *ev)
{
    // Figure out what frame this corresponds to, then set the value
    double percentageLocation {double(ev->x()) / double(this->width())};
    int frame {int(round(this->minimum() + percentageLocation * (this->maximum() - this->minimum())))};
    setValue (frame);
}


void MovieFrameSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    // Figure out what frame this corresponds to, then set the value
    double percentageLocation {double(ev->x()) / double(this->width())};
    int frame {int(round(this->minimum() + percentageLocation * (this->maximum() - this->minimum())))};
    setValue (frame);
}
