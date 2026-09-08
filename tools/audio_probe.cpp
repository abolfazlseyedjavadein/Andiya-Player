#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSink>
#include <QCoreApplication>
#include <QDebug>
#include <QMediaDevices>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QIODevice>
#include <QTimer>
#include <QUrl>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    if (application.arguments().size() < 2) {
        qCritical() << "Usage: AndiyaAudioProbe <media-file>";
        return 2;
    }

    const QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    qInfo() << "audio-output-count" << devices.size();
    for (qsizetype index = 0; index < devices.size(); ++index) {
        const QAudioFormat preferred = devices.at(index).preferredFormat();
        qInfo() << "audio-output" << index << devices.at(index).description()
                << "default" << (devices.at(index) == QMediaDevices::defaultAudioOutput())
                << "preferred" << preferred.sampleRate() << "Hz"
                << preferred.channelCount() << "channels"
                << preferred.channelConfig() << preferred.sampleFormat();
    }

    QMediaPlayer player;
    QAudioFormat playbackFormat;
    playbackFormat.setSampleRate(48000);
    playbackFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    playbackFormat.setSampleFormat(QAudioFormat::Int16);
    QAudioBufferOutput bufferOutput(playbackFormat);
    QAudioSink sink(QMediaDevices::defaultAudioOutput(), playbackFormat);
    sink.setVolume(1.0F);
    QIODevice *sinkDevice = sink.start();
    player.setAudioBufferOutput(&bufferOutput);

    quint64 decodedBuffers = 0;
    quint64 decodedFrames = 0;
    quint64 writtenBytes = 0;
    QObject::connect(&bufferOutput, &QAudioBufferOutput::audioBufferReceived,
                     &application, [&](const QAudioBuffer &buffer) {
        if (!buffer.isValid()) {
            return;
        }
        ++decodedBuffers;
        decodedFrames += static_cast<quint64>(buffer.frameCount());
        if (sinkDevice) {
            const qint64 written = sinkDevice->write(buffer.constData<char>(), buffer.byteCount());
            if (written > 0) {
                writtenBytes += static_cast<quint64>(written);
            }
        }
        if (decodedBuffers == 1) {
            qInfo() << "first-buffer" << buffer.frameCount() << "frames"
                    << buffer.format().sampleRate() << "Hz"
                    << buffer.format().channelCount() << "channels"
                    << buffer.format().sampleFormat();
        }
    });
    QObject::connect(&player, &QMediaPlayer::tracksChanged, &application, [&] {
        qInfo() << "tracks" << "audio" << player.audioTracks().size()
                << "active" << player.activeAudioTrack()
                << "video" << player.videoTracks().size();
        if (!player.audioTracks().isEmpty() && player.activeAudioTrack() < 0) {
            player.setActiveAudioTrack(0);
        }
    });
    QObject::connect(&player, &QMediaPlayer::hasAudioChanged, &application,
                     [](bool available) { qInfo() << "has-audio" << available; });
    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &application,
                     [](QMediaPlayer::MediaStatus status) { qInfo() << "media-status" << status; });
    QObject::connect(&player, &QMediaPlayer::errorOccurred, &application,
                     [](QMediaPlayer::Error error, const QString &message) {
        qCritical() << "media-error" << error << message;
    });

    QTimer::singleShot(8000, &application, [&] {
        qInfo() << "summary"
                << "device" << QMediaDevices::defaultAudioOutput().description()
                << "sink-state" << sink.state()
                << "sink-error" << sink.error()
                << "processed-us" << sink.processedUSecs()
                << "has-audio" << player.hasAudio()
                << "active-track" << player.activeAudioTrack()
                << "decoded-buffers" << decodedBuffers
                << "decoded-frames" << decodedFrames
                << "written-bytes" << writtenBytes
                << "error" << player.error() << player.errorString();
        application.quit();
    });

    player.setSource(QUrl::fromLocalFile(application.arguments().at(1)));
    player.play();
    return application.exec();
}
