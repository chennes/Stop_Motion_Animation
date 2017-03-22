#ifndef AUDIOOUTPUTSTREAM_H
#define AUDIOOUTPUTSTREAM_H

#include <QObject>


extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/common.h>
    #include <libavformat/avformat.h>
}

class AudioOutputStream : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutputStream(QObject *parent = 0);

};

#endif // AUDIOOUTPUTSTREAM_H
