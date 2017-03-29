#ifndef CAMERAMONITOR_H
#define CAMERAMONITOR_H

#include <QThread>
#include <QCameraInfo>

class CameraMonitor : public QThread
{
    Q_OBJECT

public:
    CameraMonitor(QObject *parent, QCameraInfo camera);
    void run() Q_DECL_OVERRIDE;

signals:
    void cameraLost();

private:
    QCameraInfo _camera;
};

#endif // CAMERAMONITOR_H
