#include "stopmotionanimation.h"
#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set up the name of the app so we can use QSettings via its default constructor
    QCoreApplication::setOrganizationName("Pioneer Library System");
    QCoreApplication::setOrganizationDomain("pioneerlibrarysystem.org");
    QCoreApplication::setApplicationName("Stop Motion Animation");

    StopMotionAnimation w;
    w.show();

    return a.exec();
}
