#-------------------------------------------------
#
# Project created by QtCreator 2017-02-06T11:27:37
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets widgets

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
    filenameconstructiondialog.cpp \
    avcodecwrapper.cpp \
    backgroundmusicdialog.cpp \
    utils.cpp \
    waveform.cpp \
    savefinalmoviedialog.cpp

HEADERS  += stopmotionanimation.h \
    movie.h \
    soundeffect.h \
    helpdialog.h \
    settingsdialog.h \
    filenameconstructiondialog.h \
    avcodecwrapper.h \
    backgroundmusicdialog.h \
    utils.h \
    waveform.h \
    savefinalmoviedialog.h

FORMS    += stopmotionanimation.ui \
    helpdialog.ui \
    settingsdialog.ui \
    filenameconstructiondialog.ui \
    backgroundmusicdialog.ui \
    savefinalmoviedialog.ui

RESOURCES += \
    resources.qrc

RC_ICONS = PLS-SM-Icon.ico

win32: LIBS += -L$$PWD/../ffmpeg/win64/lib/ -lavutil

INCLUDEPATH += $$PWD/../ffmpeg/win64/include
DEPENDPATH += $$PWD/../ffmpeg/win64/include

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/avutil.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/libavutil.a

win32: LIBS += -L$$PWD/../ffmpeg/win64/lib/ -lavcodec

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/avcodec.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/libavcodec.a

win32: LIBS += -L$$PWD/../ffmpeg/win64/lib/ -lavformat

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/avformat.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/libavformat.a

win32: LIBS += -L$$PWD/../ffmpeg/win64/lib/ -lswscale

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/swscale.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/libswscale.a

win32: LIBS += -L$$PWD/../ffmpeg/win64/lib/ -lswresample

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/swresample.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/libswresample.a

DISTFILES +=
