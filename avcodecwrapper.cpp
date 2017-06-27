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
}

/***************************************************************************
 * C code adapted from ffmpeg examples                                     *
 **************************************************************************/

avcodecWrapper::avcodecWrapper() :
    src_samples_data (NULL),
    dst_samples_data (NULL),
    frame (NULL),
    frame_count (0)
{
}



void avcodecWrapper::AddVideoFrame (const QString &filename)
{
    _videoFrames.append(filename);
}

void avcodecWrapper::AddAudioFile (const SoundEffect &soundEffect, double tOffset)
{
    _soundEffects.append(soundEffect);
    _soundEffects.back().setStartTime(_soundEffects.back().getStartTime()+tOffset);
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
    //log_packet(fmt_ctx, pkt);
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
    AVFrame *audioFrame = av_frame_alloc();
    int ret;

    if (!audioFrame) {
        throw libavException("Error allocating an audio frame");
    }

    audioFrame->format = sample_fmt;
    audioFrame->channel_layout = channel_layout;
    audioFrame->sample_rate = sample_rate;
    audioFrame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(audioFrame, 0);
        if (ret < 0) {
            throw libavException("Error allocating an audio buffer: " + avErrorToQString(ret));
        }
    }

    return audioFrame;
}

void avcodecWrapper::open_audio(AVFormatContext *, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
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

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                            c->sample_rate, nb_samples);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        throw libavException("Could not copy the stream parameters");
    }
}


/*
 * encode one audio frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
int avcodecWrapper::write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // data and size must be 0;
    int ret;
    int got_packet = 0;

    av_init_packet(&pkt);
    c = ost->enc;

    _audioJoiner.SetFrameSize (ost->enc->frame_size);
    ret = av_frame_make_writable(ost->frame);
    ost->frame = _audioJoiner.GetNextFrame(); // This will always return a frame, even if it is silent
    ost->next_pts += ost->frame->nb_samples;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here
     */
    if (ret < 0) {
        throw libavException("Could not make the frame writable: " + avErrorToQString(ret));
    }

    AVRational tb;
    tb.num=1;
    tb.den=c->sample_rate;
    ost->frame->pts = av_rescale_q(ost->samples_count, tb, c->time_base);
    ost->samples_count += ost->frame->nb_samples;

    ret = avcodec_encode_audio2(c, &pkt, ost->frame, &got_packet);
    if (ret < 0) {
        throw libavException("Error encoding audio frame: " + avErrorToQString(ret));
    }

    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
        if (ret < 0) {
            throw libavException("Error while writing audio frame: " + avErrorToQString(ret));
        }
    }

    return 0; // This always writes data, the audio stream never ends.
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

void avcodecWrapper::open_video(AVFormatContext *, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
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
    ost->frame = NULL;
    //ost->frame = alloc_picture(c->pix_fmt);
    //if (!ost->frame) {
    //    throw libavException("Could not allocate video frame");
    //}

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
    //std::cout << "Trying to load frame " << frame_index << ": " << filename << std::endl;
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
            break;
        } else {
            throw libavException ("Error while decoding the frame: " + avErrorToQString(ret));
        }
    }
    for (unsigned int i = 0; i < fc->nb_streams; i++) {
        if (fc->streams[i]->codec->codec_id != AV_CODEC_ID_NONE) {
            avcodec_close(fc->streams[i]->codec);
        }
    }
    avformat_close_input (&fc);
    avformat_free_context(fc);
}

//AVFrame *avcodecWrapper::get_video_frame(OutputStream *ost)
//{
//    /* check if we want to generate more frames */
//    if (ost->next_pts >= _numberOfFrames) {
//        return NULL;
//    }

//    /* when we pass a frame to the encoder, it may keep a reference to it
//     * internally; make sure we do not overwrite it here */
//    if (!(ost->frame->buf[0])) {
//        av_frame_get_buffer (ost->frame, 0);
//    }
//    int ret = av_frame_make_writable(ost->frame);
//    if (ret < 0) {
//        throw libavException ("Could not make the frame writable: " + avErrorToQString(ret));
//    }


//    fill_yuv_image(ost->frame, ost->next_pts);

//    ost->frame->pts = ost->next_pts++;

//    return ost->frame;
//}

AVFrame *avcodecWrapper::get_video_frame(OutputStream *ost)
{
    /* check if we want to generate more frames */
    if (ost->next_pts >= _numberOfFrames) {
        return NULL;
    }

    AVInputFormat *iformat = NULL;
    AVFormatContext *format_ctx = NULL;
    AVCodec *codec;
    AVCodecContext *codec_ctx;
    AVFrame *frame;
    int frame_decoded, ret = 0;
    AVPacket pkt;
    AVDictionary *opt=NULL;

    QByteArray filenameBytes = _videoFrames.at(ost->next_pts).toLatin1();
    const char *filename = filenameBytes.data();

    av_init_packet(&pkt);

    iformat = av_find_input_format("image2pipe");
    if ((ret = avformat_open_input(&format_ctx, filename, iformat, NULL)) < 0) {
        return NULL;
    }

    if ((ret = avformat_find_stream_info(format_ctx, NULL)) < 0) {
        return NULL;
    }

    codec_ctx = format_ctx->streams[0]->codec;
    codec = avcodec_find_decoder(codec_ctx->codec_id);

    av_dict_set(&opt, "thread_type", "slice", 0);
    if ((ret = avcodec_open2(codec_ctx, codec, &opt)) < 0) {
        return NULL;
    }

    if (!(frame = av_frame_alloc()) ) {
        return NULL;
    }

    ret = av_read_frame(format_ctx, &pkt);
    if (ret < 0) {
        return NULL;
    }

    ret = avcodec_decode_video2(codec_ctx, frame, &frame_decoded, &pkt);
    if (ret < 0 || !frame_decoded) {
        return NULL;
    }

    // Unref the old frame:
    //if (ost->frame) {
    //    av_frame_unref (ost->frame);
    //}

    // Store the new one:
    ost->frame = frame;
    ost->frame->pts = ost->next_pts++;

    // Deallocate the extra stuff we no longer need, but keep the frame!
    //av_packet_unref(&pkt);
    //avcodec_close(codec_ctx);
    //avformat_close_input(&format_ctx);
    //av_dict_free(&opt);

    return frame;
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
    qDebug() << frame;

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

    av_packet_unref(&pkt);

    qDebug() << frame << " " << got_packet;
    return (frame || got_packet) ? 0 : 1;
}

void avcodecWrapper::close_stream(AVFormatContext *, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    if (ost->frame) {
        //av_frame_free(&ost->frame);
    }
    sws_freeContext(ost->sws_ctx);
}

/**************************************************************/
/* media file output */

void avcodecWrapper::wrapMain()
{
    OutputStream video_st, audio_st;
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVCodec *audio_codec, *video_codec;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;
    AVDictionary *opt = NULL;

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
    if (fmt->audio_codec != AV_CODEC_ID_NONE && !_soundEffects.empty()) {
        add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
        have_audio = 1;
        encode_audio = 1;
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (have_video) {
        open_video(oc, video_codec, &video_st, opt);
    }

    if (have_audio) {
        for (auto s = _soundEffects.begin(); s != _soundEffects.end(); ++s) {
            _audioJoiner.AddFile(s->getFilename(), s->getStartTime(), s->getInPoint(), s->getOutPoint(),s->getVolume());
        }
        _audioJoiner.SetFormat(audio_st.enc->sample_fmt);
        _audioJoiner.SetSampleRate(audio_st.enc->sample_rate);
        _audioJoiner.StartStream();
        open_audio(oc, audio_codec, &audio_st, opt);
    }

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

    char tsbuf[AV_TS_MAX_STRING_SIZE];
    while (encode_video) {
        qDebug() << "Encoding a frame...";
        /* select the stream to encode */
        if (encode_video &&
            (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
                                            audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
            qDebug() << "Video PTS " << av_ts_make_time_string (tsbuf, video_st.next_pts, &video_st.enc->time_base);
            encode_video = !write_video_frame(oc, &video_st);
        } else {
            qDebug() << "Audio PTS " << av_ts_make_time_string (tsbuf, audio_st.next_pts, &audio_st.enc->time_base);
            encode_audio = !write_audio_frame(oc, &audio_st);
        }
        qDebug() << "Done.";
    }
    qDebug() << "Finished encoding frames.";

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
