#include "cameramonitor.h"

CameraMonitor::CameraMonitor(QObject *parent, QCameraInfo camera) :
    QThread (parent),
    _camera (camera)
{
}

void CameraMonitor::run()
{
    qDebug() << "Starting a new check thread";
    while (true) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            return;
        }
        auto cameras = QCameraInfo::availableCameras();
        bool found (false);
        for (auto camera: cameras) {
            if (camera == _camera) {
                found = true;
            }
        }
        if (!found) {
            emit cameraLost();
            return;
        }

        if (QThread::currentThread()->isInterruptionRequested()) {
            return;
        }
        sleep(1);
    }
}
