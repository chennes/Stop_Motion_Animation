#ifndef FRAME_H
#define FRAME_H

#include <QObject>
#include <QString>
#include <QGraphicsItem>

class Frame : public QObject
{
    Q_OBJECT
public:
    explicit Frame(QObject *parent = 0);

signals:

public slots:

private:
    QString _filename;
    QList<QGraphicsItem *> _layers;

};

#endif // FRAME_H
