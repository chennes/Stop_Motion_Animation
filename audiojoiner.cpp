#include "audiojoiner.h"
#include "avexception.h"
#include <qdebug.h>

#include <iostream>

extern "C" {
    #include <libavutil/opt.h>
}

AudioJoiner::AudioJoiner() :
    QObject(NULL),
    _started (false),
    _sampleFormat (AV_SAMPLE_FMT_FLTP),
    _sampleRate (44100),
    _outputFrameSize (1024)
{
    avfilter_register_all();
    _outputFrame = av_frame_alloc();
}


void AudioJoiner::AddFile (QString filename, double start, double in, double out, double volume)
{
    if (_started) {
        throw std::logic_error ("Cannot add more files after output has started");
    }
    _files.append(AudioFile(filename, start, in, out, volume));
}


void AudioJoiner::SetFormat(AVSampleFormat format)
{
    _sampleFormat = format;
}

void AudioJoiner::SetSampleRate(int rate)
{
    _sampleRate = rate;
}

void AudioJoiner::SetFrameSize (unsigned int frameSize)
{
    _outputFrameSize = frameSize;
}


void AudioJoiner::StartStream()
{
    if (_started) {
        return;
    }

    int ret;
    char args[512];


    static const enum AVSampleFormat out_sample_fmts[] = { _sampleFormat, (AVSampleFormat)-1 };
    static const int64_t out_channel_layouts[] = { AV_CH_LAYOUT_STEREO, -1 };
    static const int out_sample_rates[] = { _sampleRate, -1 };

    AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
    AVFilter *abuffersink = avfilter_get_by_name("abuffersink");

    AVFilterInOut *inputs  = avfilter_inout_alloc();
    _filterGraph = avfilter_graph_alloc();
    if (!inputs || !_filterGraph) {
        throw PLSException("Ran out of memory allocating storage for the audio");
    }

    QString mixerInputs;
    QString filterChain;

    int fileNumber = 0;
    for (auto&& file: _files) {
        file.ais = new AudioInputStream(file.filename);
        file.outputs = avfilter_inout_alloc();

        // For convenience...
        AVCodecContext *codecContext = file.ais->GetCodecContext();

        // Chain them together so that for each audio file we have:
        // 1) Silence before the start of this sound (aeval=0,concat)
        // 2) Trimmed audio from in to out (atrim)
        // 3) Padded with silence to the total duration needed (apad)
        // 4) Volume adjusted (avolume)

        // We also convert mono files to stereo (duplicating the channels).

        // Construct the names for each filter:
        QString n (QString::number(fileNumber));
        QString inputName ("[in" + n + "]");
        QString endName("[end" + n + "]");

        // Create the input buffer filter for this file:
        if (!codecContext->channel_layout) {
            codecContext->channel_layout = av_get_default_channel_layout(codecContext->channels);
        }

        // This chunk is copied from https://ffmpeg.org/doxygen/trunk/filtering_audio_8c-example.html
        AVRational time_base = file.ais->GetTimeBase();
        snprintf(args, sizeof(args),
                "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%llx:channels=%d",
                 time_base.num, time_base.den, codecContext->sample_rate,
                 av_get_sample_fmt_name(codecContext->sample_fmt),
                 codecContext->channel_layout, codecContext->channels);
        ret = avfilter_graph_create_filter(&file.bufferSourceContext, abuffersrc, QString("bufSrc"+n).toLatin1().data(),
                                           args, NULL, _filterGraph);
        if (ret < 0) {
            throw AVException ("avfilter_graph_create_filter", ret);
        }

        int nChannels = codecContext->channels;
        QString channelDelays (QString::number(1000*file.start) + "|" +
                               QString::number(1000*file.start));

        file.outputs->name = av_strdup(QString("in"+n).toLatin1().data());
        file.outputs->filter_ctx = file.bufferSourceContext;
        file.outputs->pad_idx    = 0;
        file.outputs->next       = NULL;

        // Construct the argument strings for each filter:
        QString trimArgs(QString ("atrim=") +
                         "start=" + QString::number(file.in) + ":"
                         "end=" + QString::number(file.out));
        QString delayArgs(QString ("adelay=") + channelDelays);
        QString padArgs("apad"); // No need to specify any parameters
        QString volumeArgs(QString ("volume=") +
                           "volume=" + QString::number(file.volume));

        // Concatenate them into a single branch of the filter graph and store it
        filterChain.append (inputName);
        if (nChannels == 1) {
            QString sa = "[splita"+n+"]";
            QString sb = "[splitb"+n+"]";
            filterChain.append (QString("asplit") + sa + sb + "," + sa + sb + "amerge,");
        }
        filterChain.append (trimArgs +",");
        if (file.start > 0) {
            filterChain.append (delayArgs +",");
        }
        filterChain.append (padArgs +",");
        filterChain.append (volumeArgs);
        //filterChain.append ("anull");
        filterChain.append (endName + ";");

        // Also store the name of the end of the chain so we can use it in the mixer later
        mixerInputs.append(endName);

        fileNumber++;
    }

    // Link up the linked list of output buffers
    for (int f = 0; f < fileNumber-1; f++) {
        _files[f].outputs->next = _files[f+1].outputs;
    }

    // Finally, create the mixer:
    if (fileNumber > 1) {
        QString mixerArgs (mixerInputs + " amix=inputs=" + QString::number(fileNumber) + " [mixer]");
        filterChain.append(mixerArgs);
    } else {
        filterChain.append(mixerInputs + "anull" + "[mixer]");
    }

    std::cout << "Final filter: \n" << filterChain.toStdString() << std::endl;

    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&_bufferSinkContext, abuffersink, "out",
                                       NULL, NULL, _filterGraph);
    if (ret < 0) {
        throw AVException("avfilter_graph_create_filter",ret);
    }
    ret = av_opt_set_int_list(_bufferSinkContext, "sample_fmts", out_sample_fmts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        throw AVException("av_opt_set_int_list",ret);
    }
    ret = av_opt_set_int_list(_bufferSinkContext, "channel_layouts", out_channel_layouts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        throw AVException("av_opt_set_int_list",ret);
    }
    ret = av_opt_set_int_list(_bufferSinkContext, "sample_rates", out_sample_rates, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        throw AVException("av_opt_set_int_list",ret);
    }

    inputs->name       = av_strdup("mixer");
    inputs->filter_ctx = _bufferSinkContext;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    ret = avfilter_graph_parse_ptr(_filterGraph, filterChain.toLatin1().data(),
                                   &inputs, &_files[0].outputs, NULL);
    if (ret < 0) {
        throw AVException("avfilter_graph_parse_ptr", ret);
    }

    // Make sure we don't have any open inputs or outputs after parsing: this should
    // be a closed graph:
    AVFilterInOut *f = inputs;
    while (f) {
        std::cout << "Open input " << f->name << std::endl;
        f = inputs->next;
    }
    f= _files[0].outputs;
    while (f) {
        std::cout << "Open output " << f->name << std::endl;
        f = inputs->next;
    }

    ret = avfilter_graph_config(_filterGraph, NULL);
    if (ret < 0) {
        throw AVException("avfilter_graph_config", ret);
    }


    const AVFilterLink *outlink = _bufferSinkContext->inputs[0];
    av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
    av_log(NULL, AV_LOG_INFO, "Filter output --> Sample rate: %dHz, Format: %s, Layout: %s\n",
           (int)outlink->sample_rate,
           (char *)av_x_if_null(av_get_sample_fmt_name((AVSampleFormat)outlink->format), "?"),
           args);

    avfilter_inout_free(&inputs);
}

// Get a reference-counted frame from the filtered output
AVFrame* AudioJoiner::GetNextFrame()
{
    // Try the naive way for now (pushing all the frames into the buffer every time,
    // instead of checking to see if this file is actually needed right now:
    int ret = 0, ret2=0;
    while (true) {
        ret = AVERROR(EAGAIN);
        while (ret == AVERROR(EAGAIN)) {
            // Start by checking to see if we even need to read another frame of audio information
            // from the files to get another frame...
            qDebug() << "Asking filtergraph for another frame...";
            av_buffersink_set_frame_size (_bufferSinkContext, _outputFrameSize);
            ret = av_buffersink_get_frame(_bufferSinkContext, _outputFrame);
            qDebug() << "Filtergraph said " << (ret == AVERROR(EAGAIN) ? "\"Feed me!\"" : "\"OK\"");
            if (ret == AVERROR(EAGAIN)) {
                // We need more data: we don't know which file is the holdup, so just load one more
                // frame from all of them
                qDebug() << "Getting the next audio frame";
                for (auto&& file: _files) {
                    AVFrame *frame = file.ais->GetNextFrame();
                    if (frame) {
                        ret2 = av_buffersrc_add_frame_flags(file.bufferSourceContext, frame, 0);
                        if (ret2 < 0) {
                            throw AVException("av_buffersrc_add_frame_flags",ret);
                        }
                    } else {
                        ret2 = av_buffersrc_add_frame_flags(file.bufferSourceContext, NULL, 0);
                        if (ret2 < 0) {
                            throw AVException("av_buffersrc_add_frame_flags",ret);
                        }
                    }
                }
            }
        }

        if (ret == AVERROR_EOF) {
            // We could not get another frame. This should not be able to happen with the
            // real filter chain, which has an apad at the end of it.
            throw PLSException ("Unknown internal error occurred in filtering the audio");
        }
        if (ret < 0){
            throw AVException("av_buffersink_get_frame",ret);
        } else {
            qDebug() << "Returning an audio frame";
            return _outputFrame;
        }
    }
}

AVRational AudioJoiner::GetTimebase()
{
    return _bufferSinkContext->inputs[0]->time_base;
}

AudioJoiner::~AudioJoiner()
{
    // Free anything we need to free...
}
