#pragma once

#include <QAudioFormat>
#include <QByteArray>
#include <memory>

class QIODevice;

// Changes tempo without changing pitch, retaining PCM until the sink accepts it.
class AudioPlaybackBuffer {
public:
    AudioPlaybackBuffer();
    ~AudioPlaybackBuffer();
    void append(const QByteArray &pcm, const QAudioFormat &format, double rate);
    void finish();
    bool drain(QIODevice *device);
    void clear();
    qsizetype queuedBytes() const { return m_pending.size() - m_written; }

private:
    struct StretchState;
    void stretch(bool final);
    std::unique_ptr<StretchState> m_stretch;
    QAudioFormat m_format;
    double m_rate = 1.0;
    QByteArray m_input;
    QByteArray m_pending;
    qsizetype m_written = 0;
};
