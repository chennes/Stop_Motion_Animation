#-------------------------------------------------
#
# Project created by QtCreator 2017-02-06T11:27:37
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Stop_Motion_Animation
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        stopmotionanimation.cpp \
    movie.cpp \
    soundeffect.cpp \
    helpdialog.cpp \
    settingsdialog.cpp \
    filenameconstructiondialog.cpp

HEADERS  += stopmotionanimation.h \
    movie.h \
    soundeffect.h \
    helpdialog.h \
    settingsdialog.h \
    filenameconstructiondialog.h

FORMS    += stopmotionanimation.ui \
    helpdialog.ui \
    settingsdialog.ui \
    filenameconstructiondialog.ui

RESOURCES += \
    resources.qrc
