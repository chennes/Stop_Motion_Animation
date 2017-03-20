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
    StopMotionAnimation w;
    splash.show();
    QTime now;
    now.start();
    while (now.elapsed() < SPLASH_SECONDS*1000 &&
           splash.isVisible()) {
        // This is here to handle mouse clicks, which will dismiss the window
        a.processEvents();
    }

    w.show();
    splash.finish(&w);

    return a.exec();
}

bool readJSONFile(QIODevice &device, QSettings::SettingsMap &map);

bool writeJSONFile(QIODevice &device, const QSettings::SettingsMap &map);

void ConfigureSettings()
{

    // Set up a JSON-formatted writer for our settings so that we can store them locally when we are
    // running off of a thumb drive. This will enable us to store a user-readable file next to the .exe
    // so that the settings can easily be duplicated to other drives (e.g. to set up the default settings
    // for a Stop Motion program at the library...)

}
