#include "stopmotionanimation.h"
#include <QApplication>
#include <QCoreApplication>
#include <QSplashScreen>
#include <QTime>
#include <QString>
#include <QSettings>

const double SPLASH_SECONDS = 2;

void ConfigureSettings ();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Pioneer Library System");
    QCoreApplication::setOrganizationDomain("pioneerlibrarysystem.org");
    QCoreApplication::setApplicationName("PLS Stop Motion Creator");

    // Show the splashscreen:
    QPixmap pixmap(":/images/splashscreen.png");
    QSplashScreen splash(pixmap);
    splash.show();
    a.processEvents();
    QTime now;
    now.start();
    StopMotionAnimation w;
    while (now.elapsed() < SPLASH_SECONDS*1000 &&
           splash.isVisible()) {
        // This is here to handle mouse clicks, which will dismiss the window
        a.processEvents();
    }

    w.show();
    splash.finish(&w);

    return a.exec();
}
