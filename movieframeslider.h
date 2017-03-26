#ifndef MOVIEFRAMESLIDER_H
#define MOVIEFRAMESLIDER_H

#include <QSlider>

class MovieFrameSlider : public QSlider
{
public:
    MovieFrameSlider(QWidget *parent = Q_NULLPTR);

protected:

    virtual void mousePressEvent(QMouseEvent *ev);
    virtual void mouseMoveEvent(QMouseEvent *ev);
    virtual void mouseReleaseEvent(QMouseEvent *ev);

};

#endif // MOVIEFRAMESLIDER_H
