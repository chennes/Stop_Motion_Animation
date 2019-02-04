#include "audioinputstream.h"
#include "plsexception.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/common.h>
    #include <libavutil/timestamp.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/mathematics.h>
    #include <libavutil/samplefmt.h>
    #include <libavutil/opt.h>
    #include <libavutil/mathematics.h>
    #include <libavformat/avformat.h>
}

AudioInputStream::AudioInputStream(const QString &filename) :
    QObject(nullptr),
    _codec(nullptr),
    _formatContext(nullptr),
    _codecContext(nullptr),
    _index(-1)
{
    av_register_all();
    _frame = av_frame_alloc();

    int ret = 0;
    ret = avformat_open_input(&_formatContext, filename.toLatin1().data(), nullptr, nullptr); CheckAndThrow(ret);
    ret = avformat_find_stream_info(_formatContext, nullptr);CheckAndThrow(ret);
    _index = av_find_best_stream(_formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &_codec, 0); CheckAndThrow(_index);
    _codecContext = _formatContext->streams[_index]->codec;
    av_opt_set_int(_codecContext, "refcounted_frames", 1, 0);
    _codecContext->request_sample_fmt = av_get_alt_sample_fmt(_codecContext->sample_fmt, 0); // Request planar data
    ret = avcodec_open2(_codecContext, _codec, nullptr); CheckAndThrow(ret);
    av_init_packet(&_packet);
    av_init_packet(&_nullPacket);
}

AVFrame *AudioInputStream::GetNextFrame()
{
    int ret = 0;
    while (true) {
        // See if the buffer already has frames in it:
        av_frame_make_writable (_frame);
        ret = avcodec_receive_frame(_codecContext, _frame);
        if (ret == AVERROR(EAGAIN)) {
            // Nope... no frames, push in more data
            ret = av_read_frame (_formatContext, &_packet);
            if (ret == AVERROR_EOF) {
                avcodec_send_packet(_codecContext, nullptr);
            } else if (ret != AVERROR(EAGAIN)){
                CheckAndThrow(ret);
                ret = avcodec_send_packet(_codecContext, &_packet);
                CheckAndThrow(ret);
            }
        } else if (ret == AVERROR_EOF) {
            return nullptr;
        } else {
            CheckAndThrow(ret);
            return _frame;
        }
    }
}


AudioInputStream::~AudioInputStream() {
    avformat_close_input(&_formatContext);
    av_frame_free(&_frame);
}

AVCodecContext *AudioInputStream::GetCodecContext()
{
    return _codecContext;
}

AVFormatContext *AudioInputStream::GetFormatContext()
{
    return _formatContext;
}


void AudioInputStream::CheckAndThrow(int ret)
{
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        throw PLSException (errbuf);
    }
}

AVRational AudioInputStream::GetTimeBase()
{
    return _formatContext->streams[_index]->time_base;
}
