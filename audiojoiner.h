#ifndef AUDIOJOINER_H
#define AUDIOJOINER_H

#include <QObject>
#include <QString>
#include "audioinputstream.h"
#include <memory>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/common.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
    #include <libavfilter/avfiltergraph.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
}

class AudioJoiner : public QObject
{
    Q_OBJECT
public:
    explicit AudioJoiner();
    ~AudioJoiner();

    void AddFile (QString filename, double start, double in, double out, double volume);

    void SetFormat (AVSampleFormat format);

    void SetSampleRate (int rate);

    void SetFrameSize (unsigned int frameSize);

    void StartStream();

    AVFrame* GetNextFrame();

    AVRational GetTimebase();

signals:

public slots:

private:
    struct AudioFile {
        AudioFile (QString f, double s, double i, double o, double v) :
            filename (f),
            start(s),
            in(i),
            out(o),
            volume(v),
            ais(nullptr),
            outputs(nullptr),
            bufferSourceContext(nullptr)
        {}

        ~AudioFile ()
        {
            if (ais) delete ais;
            //if (outputs) avfilter_inout_free(&outputs);
        }

        QString filename;

        double start;
        double in;
        double out;
        double volume;

        AudioInputStream *ais;
        AVFilterInOut *outputs;
        AVFilterContext *bufferSourceContext;

    };

    bool _started;
    QList<AudioFile> _files;

    AVSampleFormat _sampleFormat;
    int _sampleRate;
    unsigned int _outputFrameSize;
    AVFrame *_outputFrame;

    // Variables for the filter chain:
    AVFilterContext *_bufferSinkContext;
    AVFilterGraph *_filterGraph;
};

#endif // AUDIOJOINER_H
