#ifndef AVCODECWRAPPER_H
#define AVCODECWRAPPER_H

#include <QObject>
#include <QString>
#include <QException>

#include "soundeffect.h"

extern "C" {
    #include <libavformat/avformat.h>
}


class avcodecWrapper : public QObject
{
public:
    avcodecWrapper();

    void AddVideoFrame (const QString &filename);

    void AddAudioFile (const SoundEffect &soundEffect);

    void Encode (const QString &filename, int w, int h, int fps);


private:

    QStringList _videoFrames;
    QList<SoundEffect> _soundEffects;

    QString _outputFilename;
    int _w;
    int _h;
    int _framesPerSecond;
    int _numberOfFrames;
    double _streamDuration;

private:
    // A utility wrapper around a single output AVStream, used internally only
    class OutputStream {
    public:
        AVStream *st;
        AVCodecContext *enc;

        /* pts of the next frame that will be generated */
        int64_t next_pts;
        int samples_count;

        AVFrame *frame;
        AVFrame *tmp_frame;

        float t, tincr, tincr2;

        struct SwsContext *sws_ctx;
        struct SwrContext *swr_ctx;
    };


private:

    // These are basically the wrapped functions from the libav* examples, slightly
    // modified to be member functions.
    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
    int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);
    void add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, AVCodecID codec_id);
    AVFrame *alloc_audio_frame(AVSampleFormat sample_fmt,
                               uint64_t channel_layout,
                               int sample_rate, int nb_samples);
    void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
    AVFrame *get_audio_frame(OutputStream *ost);
    int write_audio_frame(AVFormatContext *oc, OutputStream *ost);
    AVFrame *alloc_picture(AVPixelFormat pix_fmt);
    void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
    void fill_yuv_image(AVFrame *pict, int frame_index);
    AVFrame *get_video_frame(OutputStream *ost);
    int write_video_frame(AVFormatContext *oc, OutputStream *ost);
    void close_stream(AVFormatContext *oc, OutputStream *ost);
    void wrapMain();

private:

    // Formerly DEFINEs
    const int INBUF_SIZE = 4096;
    const int AUDIO_INBUF_SIZE = 20480;
    const int AUDIO_REFILL_THRESH = 4096;
    const AVPixelFormat STREAM_PIX_FMT = AV_PIX_FMT_YUV420P;

    // Audio output variables
    float t, tincr, tincr2;
    uint8_t **src_samples_data;
    int       src_samples_linesize;
    int       src_nb_samples;
    int max_dst_nb_samples;
    uint8_t **dst_samples_data;
    int       dst_samples_linesize;
    int       dst_samples_size;
    struct SwrContext *swr_ctx;

    // Video output variables
    AVFrame *frame;
    AVPicture src_picture, dst_picture;
    int frame_count;

public:

    class libavException : public QException
    {
    public:
        libavException(QString message = "") {_message = message;}
        void raise() const { throw *this; }
        libavException *clone() const {return new libavException(*this); }
        QString message() const {return _message;}
    private:
        QString _message;
    };

};

#endif // AVCODECWRAPPER_H
