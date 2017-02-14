#include "stopmotionanimation.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    StopMotionAnimation w;
    w.show();

    return a.exec();
}
