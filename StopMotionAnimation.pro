#-------------------------------------------------
#
# Project created by QtCreator 2017-02-06T11:27:37
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets widgets

TARGET = Stop_Motion_Creator
TEMPLATE = app

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp \
           stopmotionanimation.cpp \
           movie.cpp \
           soundeffect.cpp \
           helpdialog.cpp \
           settingsdialog.cpp \
           filenameconstructiondialog.cpp \
           avcodecwrapper.cpp \
           utils.cpp \
           waveform.cpp \
           savefinalmoviedialog.cpp \
           previousframeoverlayeffect.cpp \
           frameeditor.cpp \
           frame.cpp \
           settings.cpp \
           audioinputstream.cpp \
           audiojoiner.cpp \
           multiregionwaveform.cpp \
           variableselectionwaveform.cpp \
           soundselectiondialog.cpp \
           movieframeslider.cpp \
           addtopreviousmoviedialog.cpp \
           cameramonitor.cpp

HEADERS  += stopmotionanimation.h \
            movie.h \
            soundeffect.h \
            helpdialog.h \
            settingsdialog.h \
            filenameconstructiondialog.h \
            avcodecwrapper.h \
            utils.h \
            waveform.h \
            savefinalmoviedialog.h \
            previousframeoverlayeffect.h \
            frameeditor.h \
            frame.h \
            settings.h \
            audioinputstream.h \
            audiojoiner.h  \
            plsexception.h \
            avexception.h \
            multiregionwaveform.h \
            variableselectionwaveform.h \
            soundselectiondialog.h \
            movieframeslider.h \
            addtopreviousmoviedialog.h \
            cameramonitor.h

FORMS    += stopmotionanimation.ui \
            helpdialog.ui \
            settingsdialog.ui \
            filenameconstructiondialog.ui \
            savefinalmoviedialog.ui \
            frameeditor.ui \
            soundselectiondialog.ui \
            addtopreviousmoviedialog.ui

RESOURCES += \
    resources.qrc

RC_FILE = StopMotionAnimation.rc

GIT_VERSION = $$system($$quote(git describe --tags))
GIT_TIMESTAMP = $$system($$quote(git log -n 1 --format=format:"%at"))

QMAKE_SUBSTITUTES += $$PWD/version.h.in

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

win32: LIBS += -L$$PWD/../ffmpeg/win64/lib/ -lavfilter

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/avfilter.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/libavfilter.a

win32: LIBS += -L$$PWD/../ffmpeg/win64/lib/ -lswscale

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/swscale.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/libswscale.a

win32: LIBS += -L$$PWD/../ffmpeg/win64/lib/ -lswresample

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/swresample.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../ffmpeg/win64/lib/libswresample.a

DISTFILES += \
    StopMotionAnimation.rc \
    version.h.in

