// The following code is a C++ wrapper around the examples from ffmpeg.org...

/*
 * doc/examples/muxing.c
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "avcodecwrapper.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <iostream>

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
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
}

/***************************************************************************
 * C code adapted from ffmpeg examples                                     *
 **************************************************************************/

avcodecWrapper::avcodecWrapper() :
    t (0.0),
    src_samples_data (NULL),
    dst_samples_data (NULL),
    swr_ctx (NULL),
    frame (NULL),
    frame_count (0)
{
}



void avcodecWrapper::AddVideoFrame (const QString &filename)
{
    _videoFrames.append(filename);
}

void avcodecWrapper::AddAudioFile (const SoundEffect &soundEffect)
{
    _soundEffects.append(soundEffect);
}

void avcodecWrapper::Encode (const QString &filename, int w, int h, int fps)
{
    _outputFilename = filename;
    _numberOfFrames = _videoFrames.length();
    _w = w;
    _h = h;
    _framesPerSecond = fps;
    _streamDuration = (double)_numberOfFrames / (double)_framesPerSecond;

    // Eventually move the necesary contents of wrapMain to here...
    wrapMain();
}

// Quick utility function...
QString avErrorToQString(int err) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(err, errbuf, AV_ERROR_MAX_STRING_SIZE);
    QString errorMessage = QString::fromUtf8(errbuf);
    return errorMessage;
}


/**
 * @file
 * libavformat API example.
 *
 * Output a media file in any supported libavformat format. The default
 * codecs are used.
 * @example muxing.c
 */

void avcodecWrapper::log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    char s1[AV_TS_MAX_STRING_SIZE];
    char s2[AV_TS_MAX_STRING_SIZE];
    char s3[AV_TS_MAX_STRING_SIZE];
    char ts1[AV_TS_MAX_STRING_SIZE];
    char ts2[AV_TS_MAX_STRING_SIZE];
    char ts3[AV_TS_MAX_STRING_SIZE];
    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts_make_string(s1, pkt->pts),
           av_ts_make_time_string(ts1, pkt->pts, time_base),
           av_ts_make_string(s2,pkt->dts),
           av_ts_make_time_string(ts2, pkt->dts, time_base),
           av_ts_make_string(s3,pkt->duration),
           av_ts_make_time_string(ts3, pkt->duration, time_base),
           pkt->stream_index);
}

int avcodecWrapper::write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    log_packet(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
void avcodecWrapper::add_stream(OutputStream *ost, AVFormatContext *oc,
                       AVCodec **codec,
                       enum AVCodecID codec_id)
{
    AVCodecContext *c;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        throw libavException("Could not find encoder for '" + QString::fromUtf8(avcodec_get_name(codec_id)) + "'");
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        throw libavException("Could not allocate stream");
    }
    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        throw libavException("Could not allocate an encoding context");
    }
    ost->enc = c;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 44100)
                    c->sample_rate = 44100;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        if ((*codec)->channel_layouts) {
            c->channel_layout = (*codec)->channel_layouts[0];
            for (i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        ost->st->time_base.num = 1;
        ost->st->time_base.den = c->sample_rate;
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;
        /* Resolution must be a multiple of two. */
        c->width    = _w;
        c->height   = _h;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        ost->st->time_base.num = 1;
        ost->st->time_base.den = _framesPerSecond;
        c->time_base       = ost->st->time_base;
        c->pix_fmt       = STREAM_PIX_FMT;

        if (codec_id == AV_CODEC_ID_H264) {
            av_opt_set(c->priv_data, "preset", "slow", 0);
        }
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B-frames */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
        }
    break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

/**************************************************************/
/* audio output */

AVFrame *avcodecWrapper::alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        throw libavException("Error allocating an audio frame");
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            throw libavException("Error allocating an audio buffer: " + avErrorToQString(ret));
        }
    }

    return frame;
}

void avcodecWrapper::open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = NULL;

    c = ost->enc;

    /* open it */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        throw libavException("Could not open audio codec: " + avErrorToQString(ret));
    }

    /* init signal generator */
    ost->t     = 0;
    ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                       c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       c->sample_rate, nb_samples);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        throw libavException("Could not copy the stream parameters");
    }

    /* create resampler context */
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx) {
        throw libavException("Could not allocate resampler context");
    }

    /* set options */
    av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

    /* initialize the resampling context */
    if ((ret = swr_init(ost->swr_ctx)) < 0) {
        throw libavException("Failed to initialize the resampling context: " + avErrorToQString(ret));
    }
}

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
AVFrame *avcodecWrapper::get_audio_frame(OutputStream *ost)
{
    AVFrame *frame = ost->tmp_frame;
    int j, i, v;
    int16_t *q = (int16_t*)frame->data[0];

    /* check if we want to generate more frames */
    AVRational oneOverOne;
    oneOverOne.num = 1;
    oneOverOne.den = 1;
    if (av_compare_ts(ost->next_pts, ost->enc->time_base,
                      _streamDuration, oneOverOne) >= 0)
        return NULL;

    for (j = 0; j <frame->nb_samples; j++) {
        v = (int)(sin(ost->t) * 10000);
        for (i = 0; i < ost->enc->channels; i++)
            *q++ = v;
        ost->t     += ost->tincr;
        ost->tincr += ost->tincr2;
    }

    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;

    return frame;
}

/*
 * encode one audio frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
int avcodecWrapper::write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // data and size must be 0;
    AVFrame *frame;
    int ret;
    int got_packet;
    int dst_nb_samples;

    av_init_packet(&pkt);
    c = ost->enc;

    frame = get_audio_frame(ost);

    if (frame) {
        /* convert samples from native format to destination codec format, using the resampler */
            /* compute destination number of samples */
            dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                            c->sample_rate, c->sample_rate, AV_ROUND_UP);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0) {
            throw libavException("Could not make the frame writable: " + avErrorToQString(ret));
        }

        /* convert to destination format */
        ret = swr_convert(ost->swr_ctx,
                          ost->frame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            throw libavException("Error while converting: " + avErrorToQString(ret));
        }
        frame = ost->frame;

        AVRational tb;
        tb.num=1;
        tb.den=c->sample_rate;
        frame->pts = av_rescale_q(ost->samples_count, tb, c->time_base);
        ost->samples_count += dst_nb_samples;
    }

    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        throw libavException("Error encoding audio frame: " + avErrorToQString(ret));
    }

    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
        if (ret < 0) {
            throw libavException("Error while writing audio frame: " + avErrorToQString(ret));
        }
    }

    return (frame || got_packet) ? 0 : 1;
}

/**************************************************************/
/* video output */

AVFrame *avcodecWrapper::alloc_picture(enum AVPixelFormat pix_fmt)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width  = _w;
    picture->height = _h;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        throw libavException("Could not allocate frame data: " + avErrorToQString(ret));
    }

    return picture;
}

void avcodecWrapper::open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        throw libavException("Could not open video codec: " + avErrorToQString(ret));
    }

    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt);
    if (!ost->frame) {
        throw libavException("Could not allocate video frame");
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P);
        if (!ost->tmp_frame) {
            throw libavException("Could not allocate temporary picture");
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        throw libavException("Could not copy the stream parameters: " + avErrorToQString(ret));
    }
}

/* Prepare a dummy image. */
void avcodecWrapper::fill_yuv_image(AVFrame *pict, int frame_index)
{
    AVFormatContext *fc = avformat_alloc_context();
    AVDictionary *opt = NULL;
    QByteArray filenameBytes = _videoFrames.at(frame_index).toLatin1();
    const char *filename = filenameBytes.data();

    int ret;
    std::cout << "Trying to load frame " << frame_index << ": " << filename << std::endl;
    ret = avformat_open_input (&fc, filename, NULL, &opt);
    if (ret < 0) {
        throw libavException("Error reading the video frame file:" + avErrorToQString(ret));
    }
    av_dump_format(fc, 0, filename, false);

    AVCodecContext *cc;
    cc = fc->streams[0]->codec;
    cc->width = _w;
    cc->height = _h;

    AVCodec *c = avcodec_find_decoder(cc->codec_id);
    if (!c){
        throw libavException ("Could not load the decoder for the image files");
    }

    ret = avcodec_open2(cc, c, &opt);
    if (ret < 0) {
        throw libavException("Could not open the appropriate codec for the image files:" + avErrorToQString(ret));
    }

    int frameFinished;

    AVPacket packet;
    av_init_packet(&packet);
    while (av_read_frame(fc, &packet) >= 0) {
        if(packet.stream_index != 0) {
            continue;
        }

        int ret = avcodec_decode_video2(cc, pict, &frameFinished, &packet);
        if (ret > 0) {
            pict->quality = 1;
            return;
        } else {
            throw libavException ("Error while decoding the frame: " + avErrorToQString(ret));
        }
    }
}

AVFrame *avcodecWrapper::get_video_frame(OutputStream *ost)
{
    AVCodecContext *c = ost->enc;

    /* check if we want to generate more frames */
    if (ost->next_pts >= _numberOfFrames) {
        return NULL;
    }

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (!(ost->frame->buf[0])) {
        av_frame_get_buffer (ost->frame, 0);
    }
    int ret = av_frame_make_writable(ost->frame);
    if (ret < 0) {
        throw libavException ("Could not make the frame writable: " + avErrorToQString(ret));
    }

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        /* as we only generate a YUV420P picture, we must convert it
         * to the codec pixel format if needed */
        if (!ost->sws_ctx) {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                                          AV_PIX_FMT_YUV420P,
                                          c->width, c->height,
                                          c->pix_fmt,
                                          SWS_BICUBIC, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                throw libavException("Could not initialize the conversion context");
            }
        }
        fill_yuv_image(ost->tmp_frame, ost->next_pts);
        sws_scale(ost->sws_ctx,
                  (const uint8_t * const *)ost->tmp_frame->data, ost->tmp_frame->linesize,
                  0, c->height, ost->frame->data, ost->frame->linesize);
    } else {
        fill_yuv_image(ost->frame, ost->next_pts);
    }

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}

/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
int avcodecWrapper::write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    int got_packet = 0;
    AVPacket pkt = { 0 };

    c = ost->enc;

    frame = get_video_frame(ost);

    av_init_packet(&pkt);

    /* encode the image */
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        throw libavException("Error encoding video frame:" + avErrorToQString(ret));
    }

    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
    } else {
        ret = 0;
    }

    if (ret < 0) {
        throw libavException("Error while writing video frame: " + avErrorToQString(ret));
    }

    return (frame || got_packet) ? 0 : 1;
}

void avcodecWrapper::close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}

/**************************************************************/
/* media file output */

void avcodecWrapper::wrapMain()
{
    OutputStream video_st = { 0 }, audio_st = { 0 };
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVCodec *audio_codec, *video_codec;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;
    AVDictionary *opt = NULL;
    int i;

    /* Initialize libavcodec, and register all codecs and formats. */
    av_register_all();

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, _outputFilename.toUtf8().data());
    if (!oc) {
        throw libavException("Could not deduce file type from filename: " + _outputFilename);
    }

    fmt = oc->oformat;
    fmt->video_codec = AV_CODEC_ID_H264;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
        have_audio = 1;
        encode_audio = 1;
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (have_video)
        open_video(oc, video_codec, &video_st, opt);

    if (have_audio)
        open_audio(oc, audio_codec, &audio_st, opt);

    av_dump_format(oc, 0, _outputFilename.toUtf8(), 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, _outputFilename.toUtf8().data(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            throw libavException("Could not open file for writing: " + avErrorToQString(ret));
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        throw libavException("Error occurred when opening output file: " + avErrorToQString(ret));
    }

    while (encode_video || encode_audio) {
        /* select the stream to encode */
        if (encode_video &&
            (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
                                            audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
            encode_video = !write_video_frame(oc, &video_st);
        } else {
            encode_audio = !write_audio_frame(oc, &audio_st);
        }
    }

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    if (have_video)
        close_stream(oc, &video_st);
    if (have_audio)
        close_stream(oc, &audio_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);
}

/***************************************************************************
 * End of C code adapted from ffmpeg examples                              *
 **************************************************************************/
