#include "AudioPlaybackBuffer.h"
#include "signalsmith-stretch.h"
#include <QIODevice>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>
#include <vector>

namespace {
template<typename T> float readSample(const char *data) {
    T value;std::memcpy(&value,data,sizeof(T));
    if constexpr (std::is_floating_point_v<T>) return std::isfinite(value)?value:0;
    else if constexpr (std::is_unsigned_v<T>) return (int(value)-128)/128.0f;
    else return float(double(value)/(-double(std::numeric_limits<T>::min())));
}
template<typename T> void writeSample(char *data,float value) {
    const double sample=std::isfinite(value)?std::clamp(double(value),-1.0,1.0):0;
    T result;
    if constexpr (std::is_floating_point_v<T>) result=T(sample);
    else if constexpr (std::is_unsigned_v<T>) result=T(std::clamp(std::round(sample*128+128),0.0,255.0));
    else result=T(std::clamp(std::round(sample*(-double(std::numeric_limits<T>::min()))),
                            double(std::numeric_limits<T>::min()),double(std::numeric_limits<T>::max())));
    std::memcpy(data,&result,sizeof(T));
}
}

struct AudioPlaybackBuffer::StretchState {
    signalsmith::stretch::SignalsmithStretch<float> engine{0};
    qint64 totalInput=0,processedInput=0,outputFrames=0;
    int seekFrames=0;
    bool primed=false;
};

AudioPlaybackBuffer::AudioPlaybackBuffer()=default;
AudioPlaybackBuffer::~AudioPlaybackBuffer()=default;

void AudioPlaybackBuffer::append(const QByteArray &pcm,const QAudioFormat &format,double rate) {
    if(!format.isValid()||!std::isfinite(rate)||rate<=0||pcm.isEmpty()||pcm.size()%format.bytesPerFrame())return;
    if(format!=m_format||rate!=m_rate){finish();m_format=format;m_rate=rate;}
    // Keep normal playback byte exact and avoid unnecessary DSP latency.
    if(rate==1.0){m_pending.append(pcm);return;}
    if(!m_stretch){
        m_stretch=std::make_unique<StretchState>();
        m_stretch->engine.configure(format.channelCount(),format.sampleRate()/20,format.sampleRate()/100);
        m_stretch->engine.setTransposeFactor(1.0f);
        m_stretch->seekFrames=m_stretch->engine.outputSeekLength(rate);
    }
    m_stretch->totalInput+=pcm.size()/format.bytesPerFrame();
    m_input.append(pcm);stretch(false);
}

void AudioPlaybackBuffer::stretch(bool final) {
    if(!m_stretch)return;
    auto &state=*m_stretch;
    const int channels=m_format.channelCount(),frameBytes=m_format.bytesPerFrame(),sampleBytes=m_format.bytesPerSample();
    auto decode=[&](int frames){
        std::vector<std::vector<float>> planar(channels,std::vector<float>(frames));
        const int available=qMin(frames,int(m_input.size()/frameBytes));
        for(int i=0;i<available;++i)for(int c=0;c<channels;++c){
            const char *src=m_input.constData()+i*frameBytes+c*sampleBytes;
            switch(m_format.sampleFormat()){
            case QAudioFormat::UInt8: planar[c][i]=readSample<quint8>(src);break;
            case QAudioFormat::Int16: planar[c][i]=readSample<qint16>(src);break;
            case QAudioFormat::Int32: planar[c][i]=readSample<qint32>(src);break;
            case QAudioFormat::Float: planar[c][i]=readSample<float>(src);break;
            default:break;
            }
        }
        return planar;
    };
    auto encode=[&](const std::vector<std::vector<float>> &planar,int frames){
        QByteArray pcm(frames*frameBytes,Qt::Uninitialized);
        for(int i=0;i<frames;++i)for(int c=0;c<channels;++c){
            char *dst=pcm.data()+i*frameBytes+c*sampleBytes;
            switch(m_format.sampleFormat()){
            case QAudioFormat::UInt8:writeSample<quint8>(dst,planar[c][i]);break;
            case QAudioFormat::Int16:writeSample<qint16>(dst,planar[c][i]);break;
            case QAudioFormat::Int32:writeSample<qint32>(dst,planar[c][i]);break;
            case QAudioFormat::Float:writeSample<float>(dst,planar[c][i]);break;
            default:break;
            }
        }
        m_pending.append(pcm);state.outputFrames+=frames;
    };
    if(!state.primed){
        if(!final&&m_input.size()/frameBytes<state.seekFrames)return;
        // Prime lookahead without emitting the DSP's initial silent pre-roll.
        auto input=decode(state.seekFrames);
        state.engine.outputSeek(input,state.seekFrames);
        m_input.remove(0,qMin(m_input.size(),qsizetype(state.seekFrames)*frameBytes));
        state.processedInput=state.seekFrames;state.primed=true;
    }
    const int inputFrames=int(m_input.size()/frameBytes);
    if(inputFrames>0){
        const qint64 target=std::llround((state.processedInput-state.seekFrames+inputFrames)/m_rate);
        const int outputFrames=int(target-state.outputFrames);
        auto input=decode(inputFrames);
        std::vector<std::vector<float>> output(channels,std::vector<float>(outputFrames));
        state.engine.process(input,inputFrames,output,outputFrames);
        encode(output,outputFrames);
        state.processedInput+=inputFrames;m_input.clear();
    }
    if(final){
        const int tailFrames=int(std::llround(state.totalInput/m_rate)-state.outputFrames);
        if(tailFrames>0){
            std::vector<std::vector<float>> output(channels,std::vector<float>(tailFrames));
            state.engine.flush(output,tailFrames,float(m_rate));encode(output,tailFrames);
        }
    }
}

void AudioPlaybackBuffer::finish() {
    stretch(true);m_stretch.reset();m_input.clear();
}

bool AudioPlaybackBuffer::drain(QIODevice *device) {
    if(!device)return true;
    while(m_written<m_pending.size()){
        const auto written=device->write(m_pending.constData()+m_written,queuedBytes());
        if(written<0)return false;
        if(written==0)break;
        m_written+=written;
    }
    if(m_written==m_pending.size()){m_pending.clear();m_written=0;}
    else if(m_written>65536){m_pending.remove(0,m_written);m_written=0;}
    return true;
}

void AudioPlaybackBuffer::clear() {
    m_stretch.reset();m_input.clear();m_pending.clear();m_written=0;
}
