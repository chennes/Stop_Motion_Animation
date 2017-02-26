#include "stopmotionanimation.h"
#include <QApplication>
#include <QCoreApplication>
#include <QSplashScreen>
#include <QTime>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set up the name of the app so we can use QSettings via its default constructor
    QCoreApplication::setOrganizationName("Pioneer Library System");
    QCoreApplication::setOrganizationDomain("pioneerlibrarysystem.org");
    QCoreApplication::setApplicationName("Stop Motion Animation");

    QPixmap pixmap(":/splashscreen.png");
    QSplashScreen splash(pixmap);
    splash.show();
    a.processEvents();

    QTime now;
    now.start();
    while (now.elapsed() < 1000) {
        continue;
    }

    StopMotionAnimation w;
    w.show();
    splash.finish(&w);

    return a.exec();
}
