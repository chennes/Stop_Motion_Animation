#ifndef CAMERAMONITOR_H
#define CAMERAMONITOR_H

#include <QThread>
#include <QCameraInfo>
#include <QSemaphore>

class CameraMonitor : public QThread
{
    Q_OBJECT

public:
    CameraMonitor(QObject *parent, QCameraInfo camera);
    ~CameraMonitor() override;
    void run() Q_DECL_OVERRIDE;

signals:
    void cameraLost();

private:
    QCameraInfo _camera;
    QSemaphore _threadWait;
};

#endif // CAMERAMONITOR_H
