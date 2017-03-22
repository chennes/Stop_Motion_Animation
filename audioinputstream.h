#ifndef AUDIOINPUTSTREAM_H
#define AUDIOINPUTSTREAM_H

#include <QObject>
#include <QString>
#include <QException>


extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/common.h>
    #include <libavformat/avformat.h>
}


class AudioInputStream : public QObject
{
    Q_OBJECT
public:
    explicit AudioInputStream(const QString &filename);
    ~AudioInputStream();
    AVCodecContext *GetCodecContext();
    AVFormatContext *GetFormatContext();
    AVRational GetTimeBase();
    AVFrame * GetNextFrame();

private:
    static void CheckAndThrow(int ret);

    AVCodec *_codec;
    AVFormatContext *_formatContext;
    AVCodecContext *_codecContext;
    AVFrame *_frame;
    AVPacket _packet;
    AVPacket _nullPacket;
    int _index;
};

#endif // AUDIOINPUTSTREAM_H
