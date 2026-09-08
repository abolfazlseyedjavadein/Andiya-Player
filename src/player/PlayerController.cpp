#include "PlayerController.h"

#include "plugins/PluginListModel.h"
#include "plugins/FilterPipeline.h"
#include "FrameStore.h"
#include "ExportQueue.h"
#include "StudioController.h"
#include <QSaveFile>
#include <QPainter>
#include <QThreadPool>
#include <QPointer>
#include <QUuid>

#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QByteArray>
#include <QCryptographicHash>
#include <QMediaDevices>
#include <QMediaMetaData>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QMediaPlayer>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QSet>
#include <QTimer>
#include <QVariant>
#include <QVideoFrame>
#include <QVideoSink>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {


// Transports that are inherently live/non-seekable. Regular http(s) URLs are
// deliberately excluded: those are usually progressive downloads of a VOD
// file and can still be resumed/seeked like a local file.
bool isLiveStreamScheme(const QString &scheme)
{
    static const QStringList liveSchemes = {
        QStringLiteral("rtsp"), QStringLiteral("rtsps"), QStringLiteral("rtmp"),
        QStringLiteral("rtmps"), QStringLiteral("mms"), QStringLiteral("mmsh"),
        QStringLiteral("udp"), QStringLiteral("rtp"), QStringLiteral("srt"),
        QStringLiteral("tcp"), QStringLiteral("dvb"), QStringLiteral("satip")
    };
    return liveSchemes.contains(scheme.toLower());
}

qint64 parseSubtitleTimestamp(const QString &value)
{
    static const QRegularExpression expression(
        QStringLiteral("^(?:(\\d{1,2}):)?(\\d{2}):(\\d{2})[,.](\\d{3})"));
    const QRegularExpressionMatch match = expression.match(value.trimmed());
    if (!match.hasMatch()) {
        return -1;
    }
    const qint64 hours = match.captured(1).isEmpty() ? 0 : match.captured(1).toLongLong();
    return hours * 3'600'000 + match.captured(2).toLongLong() * 60'000 +
           match.captured(3).toLongLong() * 1'000 + match.captured(4).toLongLong();
}

float sinkVolume(qreal volume, bool muted)
{
    return muted ? 0.0F : static_cast<float>(std::min<qreal>(volume, 1.0));
}
}

PlayerController::PlayerController(PluginListModel *plugins, QObject *parent)
    : QObject(parent),
      m_player(new QMediaPlayer(this)),
      m_plugins(plugins)
{
    m_previewPipeline=new FilterPipeline(this);m_capturePipeline=new FilterPipeline(this);m_exports=new ExportQueue(this);
    connect(m_plugins,&PluginListModel::maintenanceRequested,this,[this]{
        ++m_generation;m_exports->releaseWorkers();m_previewPipeline->releaseWorkers();m_capturePipeline->releaseWorkers();
    });
    m_videoSink=new QVideoSink(this);m_player->setVideoSink(m_videoSink);
    connect(m_videoSink,&QVideoSink::videoFrameChanged,this,&PlayerController::handleDecodedFrame);
    connect(m_videoSink,&QVideoSink::videoSizeChanged,this,&PlayerController::updateVideoSize);
    connect(m_videoSink,&QVideoSink::subtitleTextChanged,this,[this](QString text){m_embeddedSubtitleText=text;updateSubtitleText(position());});
    connect(m_player,&QMediaPlayer::seekableChanged,this,&PlayerController::durationChanged);
    connect(m_exports,&ExportQueue::progress,this,[this](double value,QString message){m_batchProgress=value;m_batchStatus=message;emit batchChanged();});
    connect(m_exports,&ExportQueue::finished,this,[this](QString message){m_batchRunning=false;m_batchStatus=message;if(message.startsWith("Export complete"))m_batchProgress=1;emit batchChanged();emit notification("Export",message);});
    connect(m_exports,&ExportQueue::wrote,this,[this](QString path){m_lastCapturePath=path;emit lastCapturePathChanged();});
    auto *diagnosticTimer=new QTimer(this);diagnosticTimer->setInterval(1000);
    connect(diagnosticTimer,&QTimer::timeout,this,&PlayerController::diagnosticsChanged);diagnosticTimer->start();
    loadPersistentState();
    m_liveFilters=QSettings().value("filters/live",true).toBool();
    connect(m_plugins,&PluginListModel::filterStackChanged,this,[this]{
        ++m_generation;
        if(!m_plugins->hasEnabledImageFilters())m_previewPipeline->process({},0,{},[](QImage,QStringList){});
        requestPreview();
    });
    refreshAudioDevice();

    m_resumeTimer = new QTimer(this);
    m_resumeTimer->setInterval(5000);
    connect(m_resumeTimer, &QTimer::timeout, this, &PlayerController::saveResumePosition);
    m_resumeTimer->start();

    auto *mediaDevices = new QMediaDevices(this);
    connect(mediaDevices, &QMediaDevices::audioOutputsChanged,
            this, &PlayerController::refreshAudioDevice);

    connect(m_player, &QMediaPlayer::positionChanged, this, &PlayerController::positionChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 current) {
        if (m_loopEnabled && m_loopStart >= 0 && m_loopEnd > m_loopStart && current >= m_loopEnd) {
            seek(m_loopStart);
        }
    });
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &PlayerController::updateSubtitleText);
    connect(m_player, &QMediaPlayer::durationChanged, this, &PlayerController::durationChanged);
    connect(m_player, &QMediaPlayer::tracksChanged, this, [this] {
        if (!m_player->audioTracks().isEmpty() && m_player->activeAudioTrack() < 0) {
            m_player->setActiveAudioTrack(0);
        }
        updateTracks();
    });
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this] {
        emit playingChanged();
        if (playing()) {
            setStatusText(QStringLiteral("Playing"));
        } else if (m_hasMedia && !m_imageMode) {
            setStatusText(QStringLiteral("Paused"));
        }
    });
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        switch (status) {
        case QMediaPlayer::LoadingMedia:
            setStatusText(QStringLiteral("Loading media..."));
            break;
        case QMediaPlayer::LoadedMedia:
        case QMediaPlayer::BufferedMedia:
            setStatusText(QStringLiteral("Ready"));
            if (m_pendingResumePosition > 0 && m_player->isSeekable()) {
                const qint64 resumeAt = m_pendingResumePosition;
                m_pendingResumePosition = 0;
                seek(resumeAt);
                emit notification(QStringLiteral("Playback resumed"),
                                  QStringLiteral("Continued from your saved position."));
            }
            break;
        case QMediaPlayer::EndOfMedia:
            setStatusText(QStringLiteral("Finished"));
            saveResumePosition();
            if (m_playlistIndex >= 0 && m_playlistIndex + 1 < m_playlist.size()) {
                playPlaylistIndex(m_playlistIndex + 1);
            }
            break;
        case QMediaPlayer::InvalidMedia:
            setStatusText(QStringLiteral("Unsupported media"));
            break;
        default:
            break;
        }
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &errorText) {
                setStatusText(QStringLiteral("Playback error"));
                Q_UNUSED(errorText);
                emit notification(QStringLiteral("Playback failed"), QStringLiteral("Check the file or stream address, credentials and codec support. For a stream, try reconnecting from Workspace > Streams."));
            });
    if (m_plugins) {
        connect(m_plugins, &PluginListModel::filterStackChanged,
                this, &PlayerController::refreshImagePreview);
        // Toggling, reordering, or installing a plugin changes what the
        // filtered side of Compare Mode should show even though the
        // underlying frame hasn't changed, so it needs its own refresh
        // instead of waiting for the next decoded video frame.
        connect(m_plugins, &PluginListModel::filterStackChanged,
                this, &PlayerController::refreshCompareFrame);
    }
}

PlayerController::~PlayerController()
{
    saveResumePosition();
    m_player->setAudioBufferOutput(nullptr);
    if (m_audioSink) {
        m_audioSink->stop();
    }
}

QString PlayerController::title() const { return m_title; }
QString PlayerController::mediaKind() const { return m_mediaKind; }
QString PlayerController::statusText() const { return m_statusText; }
QUrl PlayerController::source() const { return StudioController::publicUrl(m_source); }
QUrl PlayerController::imageSource() const { return m_imageSource; }
bool PlayerController::hasMedia() const { return m_hasMedia; }
bool PlayerController::imageMode() const { return m_imageMode; }
bool PlayerController::playing() const { return m_player->playbackState() == QMediaPlayer::PlayingState; }
bool PlayerController::muted() const { return m_muted; }
qreal PlayerController::volume() const { return m_volume; }
qreal PlayerController::playbackRate() const { return m_player->playbackRate(); }
QString PlayerController::audioDeviceName() const { return m_audioDeviceName; }
QString PlayerController::captureDirectoryPath() const
{
    return QDir::toNativeSeparators(captureDirectory());
}
QString PlayerController::subtitleText() const { return m_subtitleText; }
QString PlayerController::subtitleFileName() const { return m_subtitleFileName; }
bool PlayerController::hasSubtitles() const
{
    return !m_subtitleCues.isEmpty() || m_player->activeSubtitleTrack() >= 0;
}
bool PlayerController::subtitleRtl() const
{
    static const QRegularExpression rtl(QStringLiteral("[\\x{0590}-\\x{08FF}\\x{FB1D}-\\x{FEFC}]"));
    return rtl.match(m_subtitleText).hasMatch();
}
int PlayerController::subtitleDelayMs() const { return m_subtitleDelayMs; }
int PlayerController::subtitleFontSize() const { return m_subtitleFontSize; }
QString PlayerController::subtitleColor() const { return m_subtitleColor; }
int PlayerController::subtitlePosition() const { return m_subtitlePosition; }
QVariantList PlayerController::audioTracks() const { return m_audioTracks; }
QVariantList PlayerController::subtitleTracks() const { return m_subtitleTracks; }
int PlayerController::activeAudioTrack() const { return m_player->activeAudioTrack(); }
int PlayerController::activeSubtitleTrack() const { return m_player->activeSubtitleTrack(); }
QVariantList PlayerController::recentMedia() const { return m_recentMedia; }
QVariantList PlayerController::playlist() const { return m_playlist; }
int PlayerController::playlistIndex() const { return m_playlistIndex; }
QString PlayerController::themeName() const { return m_themeName; }
int PlayerController::savedWindowX() const { return m_savedWindowX; }
int PlayerController::savedWindowY() const { return m_savedWindowY; }
int PlayerController::savedWindowWidth() const { return m_savedWindowWidth; }
int PlayerController::savedWindowHeight() const { return m_savedWindowHeight; }
bool PlayerController::savedWindowMaximized() const { return m_savedWindowMaximized; }
qint64 PlayerController::position() const { return m_imageMode ? 0 : (m_stepFreeze && m_currentFrame.isValid() ? qMax<qint64>(0, m_currentFrame.startTime() / 1000) : m_player->position()); }
qint64 PlayerController::duration() const { return m_imageMode ? 0 : m_player->duration(); }
int PlayerController::videoWidth() const { return m_videoWidth; }
int PlayerController::videoHeight() const { return m_videoHeight; }
QString PlayerController::lastCapturePath() const { return m_lastCapturePath; }
bool PlayerController::isLiveStream() const { return m_isLiveStream; }

void PlayerController::openNetworkStream(const QString &urlText)
{
    const QString trimmed = urlText.trimmed();
    if (trimmed.isEmpty()) {
        emit notification(QStringLiteral("No URL entered"),
                          QStringLiteral("Type a stream address, e.g. rtsp://host:554/stream"));
        return;
    }

    const QUrl url(trimmed, QUrl::StrictMode);
    static const QSet<QString> supportedSchemes = {
        QStringLiteral("http"), QStringLiteral("https"), QStringLiteral("ftp"), QStringLiteral("ftps"),
        QStringLiteral("rtsp"), QStringLiteral("rtsps"), QStringLiteral("rtmp"), QStringLiteral("rtmps"),
        QStringLiteral("mms"), QStringLiteral("mmsh"), QStringLiteral("udp"), QStringLiteral("rtp"),
        QStringLiteral("tcp"), QStringLiteral("srt"), QStringLiteral("dvb"), QStringLiteral("satip"),
        QStringLiteral("smb"), QStringLiteral("sftp"), QStringLiteral("nfs"), QStringLiteral("upnp"),
        QStringLiteral("dshow"), QStringLiteral("screen"), QStringLiteral("file")
    };
    const QString scheme = url.scheme().toLower();
    const bool hasEndpoint = !url.host().isEmpty() || url.port() >= 0 || !url.path().isEmpty();
    if (!url.isValid() || !supportedSchemes.contains(scheme) || !hasEndpoint) {
        emit notification(QStringLiteral("Invalid or unsupported stream URL"),
                          QStringLiteral("Use HTTP(S)/HLS, FTP, RTSP, RTMP, MMS, RTP/UDP, SRT, SMB/SFTP/NFS, DVB/SAT>IP, or a capture-source URL."));
        return;
    }

    openMedia(url);
}
bool PlayerController::compareModeActive() const { return m_compareModeActive; }
QUrl PlayerController::compareOriginalSource() const { return m_compareOriginalSource; }
QUrl PlayerController::compareFilteredSource() const { return m_compareFilteredSource; }
bool PlayerController::compareHasFilters() const
{
    return m_plugins && m_plugins->hasEnabledImageFilters();
}

void PlayerController::attachVideoOutput(QObject *videoOutput)
{
    if(!videoOutput)return;
    m_displaySink=videoOutput->property("videoSink").value<QVideoSink *>();
    if(m_displaySink && m_currentFrame.isValid())presentFrame(m_currentFrame);
}

void PlayerController::openMedia(const QUrl &url)
{
    if (!url.isValid() || url.isEmpty()) {
        return;
    }

    saveResumePosition();
    ++m_generation;++m_compareRevision;m_stepDirection=0;m_stepFreeze=false;m_currentFrame={};m_previousCandidate={};
    m_decodedFrames=m_skippedPreviews=0;m_filterError.clear();clearLoop();exitCompareMode();
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    m_source = url;
    m_title = QFileInfo(url.isLocalFile()?path:url.path()).fileName();
    if (m_title.isEmpty()) {
        m_title = url.fileName();
    }
    if (m_title.isEmpty() && !url.host().isEmpty()) {
        // Bare stream roots like "rtsp://192.168.1.5:554" have no filename
        // segment at all, so fall back to the host so the tab/title bar
        // isn't left blank.
        m_title = url.host();
    }
    m_hasMedia = true;
    m_imageMode = isImageFile(path);
    m_imageSource = m_imageMode ? url : QUrl{};
    m_originalImage = {};
    m_pendingResumePosition = 0;
    m_isLiveStream = !m_imageMode && !url.isLocalFile() && isLiveStreamScheme(url.scheme());
    clearSubtitles();

    if (m_imageMode) {
        m_player->stop();
        restartAudioSink();
        m_player->setSource(QUrl{});
        if (url.isLocalFile()) {
            m_originalImage.load(url.toLocalFile());
        }
        m_videoWidth = m_originalImage.width();
        m_videoHeight = m_originalImage.height();
        m_mediaKind = QStringLiteral("Image");
        if(m_originalImage.isNull()){m_hasMedia=false;setStatusText("Image could not be decoded");emit notification("Image error","This image is missing, corrupt, or unsupported.");}
        else setStatusText(QStringLiteral("Image ready"));
        emit positionChanged();
        emit durationChanged();
        emit playingChanged();
        emit videoSizeChanged();
    } else {
        refreshAudioDevice();
        const QString suffix = QFileInfo(path).suffix().toLower();
        static const QStringList audioExtensions = {
            QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("flac"),
            QStringLiteral("aac"), QStringLiteral("m4a"), QStringLiteral("ogg"),
            QStringLiteral("opus"), QStringLiteral("wma")
        };
        m_mediaKind = audioExtensions.contains(suffix) ? QStringLiteral("Audio")
                                                       : QStringLiteral("Video");
        m_videoWidth = 0;
        m_videoHeight = 0;
        m_pendingResumePosition = m_isLiveStream ? 0 : savedResumePosition(url);
        m_player->setSource(url);
        m_player->setPlaybackRate(playbackRate());
        m_player->play();
        if (url.isLocalFile() && m_mediaKind == QStringLiteral("Video")) {
            autoLoadSubtitle(url.toLocalFile());
        }
    }

    emit mediaChanged();
    addRecentMedia(url);
    if (m_imageMode) {
        refreshImagePreview();
    }
}

void PlayerController::togglePlayback()
{
    if (!m_hasMedia || m_imageMode) {
        return;
    }
    if(m_stepFreeze){m_player->setPosition(position());m_stepFreeze=false;m_stepDirection=0;}
    playing() ? m_player->pause() : m_player->play();
}

void PlayerController::seek(qint64 positionMs)
{
    if (!m_imageMode && m_hasMedia && seekable()) {
        ++m_generation;m_stepFreeze=false;m_stepDirection=0;
        restartAudioSink();
        m_player->setPosition(qBound<qint64>(0, positionMs, m_player->duration()));
    }
}

void PlayerController::jumpSeconds(int seconds)
{
    seek(position() + static_cast<qint64>(seconds) * 1000);
}

void PlayerController::stepFrame(int direction)
{
    if(!m_hasMedia||m_imageMode||!seekable()||direction==0||m_stepDirection)return;
    m_player->pause();restartAudioSink();
    m_stepOriginUs=m_currentFrame.isValid()?m_currentFrame.startTime():position()*1000;
    if(m_stepOriginUs<0)return;
    m_stepFreeze=false;m_stepDirection=direction>0?1:-1;m_previousCandidate={};++m_generation;
    if(direction<0 && m_stepOriginUs==0){m_stepDirection=0;m_stepFreeze=true;return;}
    m_player->setPosition(direction>0 ? m_stepOriginUs/1000 : qMax<qint64>(0,m_stepOriginUs/1000-1500));
    m_player->play();
    const auto generation=m_generation;
    QTimer::singleShot(5000,this,[this,generation]{
        if(generation!=m_generation||!m_stepDirection)return;
        m_player->pause();m_stepDirection=0;m_stepFreeze=true;
        emit notification("Frame navigation","No adjacent frame was decoded before the timeout.");
    });
}

void PlayerController::setVolume(qreal value)
{
    if (!std::isfinite(value)) return;
    const qreal bounded = qBound<qreal>(0.0, value, 2.0);
    if (qFuzzyCompare(m_volume, bounded) && (!m_muted || bounded <= 0)) {
        return;
    }
    m_volume = bounded;
    if (bounded > 0) {
        m_muted = false;
    }
    if (m_audioSink) {
        m_audioSink->setVolume(sinkVolume(m_volume, m_muted));
    }
    emit volumeChanged();
}

void PlayerController::setPlaybackRate(qreal value)
{
    if (!std::isfinite(value)) return;
    const qreal bounded = qBound<qreal>(0.25, value, 4.0);
    if (qFuzzyCompare(m_player->playbackRate(), bounded)) {
        return;
    }
    m_player->setPlaybackRate(bounded);
    restartAudioSink();
    QSettings().setValue(QStringLiteral("playback/rate"), bounded);
    emit playbackRateChanged();
}

void PlayerController::toggleMute()
{
    m_muted = !m_muted;
    if (m_audioSink) {
        m_audioSink->setVolume(sinkVolume(m_volume, m_muted));
    }
    emit volumeChanged();
}

void PlayerController::captureFrame()
{
    captureFrameInternal(true);
}

void PlayerController::captureOriginalFrame()
{
    captureFrameInternal(false);
}

void PlayerController::revealCaptureDirectory()
{
    const QString directory = captureDirectory();
    QDir().mkpath(directory);
    QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
}

void PlayerController::setCaptureDirectory(const QUrl &directoryUrl)
{
    if (!directoryUrl.isLocalFile()) {
        emit notification(QStringLiteral("Capture folder not changed"),
                          QStringLiteral("Choose a local folder."));
        return;
    }
    const QString path = QDir::cleanPath(directoryUrl.toLocalFile());
    if (path.isEmpty() || !QDir().mkpath(path)) {
        emit notification(QStringLiteral("Capture folder not changed"),
                          QStringLiteral("Andiya could not use that folder."));
        return;
    }
    m_captureDirectory = path;
    QSettings().setValue(QStringLiteral("capture/directory"), path);
    emit captureDirectoryChanged();
    emit notification(QStringLiteral("Capture folder changed"),
                      QDir::toNativeSeparators(path));
}

void PlayerController::resetCaptureDirectory()
{
    m_captureDirectory.clear();
    QSettings().remove(QStringLiteral("capture/directory"));
    emit captureDirectoryChanged();
}

void PlayerController::openSubtitle(const QUrl &url)
{
    if (!url.isLocalFile()) {
        emit notification(QStringLiteral("Subtitle not loaded"),
                          QStringLiteral("Choose a local .srt or .vtt file."));
        return;
    }

    QFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        emit notification(QStringLiteral("Subtitle not loaded"), file.errorString());
        return;
    }

    QString contents = QString::fromUtf8(file.readAll());
    contents.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    contents.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    const QStringList blocks = contents.split(
        QRegularExpression(QStringLiteral("\\n[ \\t]*\\n+")), Qt::SkipEmptyParts);

    QVector<SubtitleCue> cues;
    for (const QString &block : blocks) {
        const QStringList lines = block.split(QLatin1Char('\n'));
        int timingLine = -1;
        for (int index = 0; index < lines.size(); ++index) {
            if (lines.at(index).contains(QStringLiteral("-->"))) {
                timingLine = index;
                break;
            }
        }
        if (timingLine < 0) {
            continue;
        }
        const QStringList range = lines.at(timingLine).split(QStringLiteral("-->"));
        if (range.size() != 2) {
            continue;
        }
        const qint64 start = parseSubtitleTimestamp(range.at(0));
        const qint64 end = parseSubtitleTimestamp(range.at(1));
        if (start < 0 || end <= start) {
            continue;
        }
        QStringList textLines = lines.mid(timingLine + 1);
        QString text = textLines.join(QLatin1Char('\n')).trimmed();
        text.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
        text.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
        text.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        text.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
        text.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
        if (!text.isEmpty()) {
            cues.push_back({start, end, text});
        }
    }

    if (cues.isEmpty()) {
        emit notification(QStringLiteral("Subtitle not loaded"),
                          QStringLiteral("No valid subtitle cues were found."));
        return;
    }

    std::sort(cues.begin(), cues.end(), [](const SubtitleCue &left, const SubtitleCue &right) {
        return left.startMs < right.startMs;
    });
    m_subtitleCues = std::move(cues);
    m_subtitleFileName = QFileInfo(url.toLocalFile()).fileName();
    emit subtitleFileChanged();
    updateSubtitleText(position());
    emit notification(QStringLiteral("Subtitles loaded"), m_subtitleFileName);
}

void PlayerController::clearSubtitles()
{
    const bool hadFile = !m_subtitleCues.isEmpty() || !m_subtitleFileName.isEmpty();
    m_subtitleCues.clear();
    m_subtitleFileName.clear();
    m_embeddedSubtitleText.clear();
    if (!m_subtitleText.isEmpty()) {
        m_subtitleText.clear();
        emit subtitleTextChanged();
    }
    if (hadFile) {
        emit subtitleFileChanged();
    }
    if (m_player->activeSubtitleTrack() >= 0) {
        m_player->setActiveSubtitleTrack(-1);
        emit tracksChanged();
    }
}

void PlayerController::setSubtitleDelayMs(int value)
{
    value = qBound(-10'000, value, 10'000);
    if (m_subtitleDelayMs == value) {
        return;
    }
    m_subtitleDelayMs = value;
    QSettings().setValue(QStringLiteral("subtitles/delayMs"), value);
    updateSubtitleText(position());
    emit subtitleSettingsChanged();
}

void PlayerController::setSubtitleFontSize(int value)
{
    value = qBound(12, value, 42);
    if (m_subtitleFontSize == value) {
        return;
    }
    m_subtitleFontSize = value;
    QSettings().setValue(QStringLiteral("subtitles/fontSize"), value);
    emit subtitleSettingsChanged();
}

void PlayerController::setSubtitleColor(const QString &value)
{
    static const QRegularExpression colorExpression(QStringLiteral("^#[0-9A-Fa-f]{6}$"));
    if (!colorExpression.match(value).hasMatch() || m_subtitleColor == value) {
        return;
    }
    m_subtitleColor = value.toUpper();
    QSettings().setValue(QStringLiteral("subtitles/color"), m_subtitleColor);
    emit subtitleSettingsChanged();
}

void PlayerController::setSubtitlePosition(int value)
{
    value = qBound(4, value, 36);
    if (m_subtitlePosition == value) {
        return;
    }
    m_subtitlePosition = value;
    QSettings().setValue(QStringLiteral("subtitles/position"), value);
    emit subtitleSettingsChanged();
}

void PlayerController::setActiveAudioTrack(int index)
{
    if (index < -1 || index >= m_player->audioTracks().size()) {
        return;
    }
    m_player->setActiveAudioTrack(index);
    emit tracksChanged();
}

void PlayerController::setActiveSubtitleTrack(int index)
{
    if (index < -1 || index >= m_player->subtitleTracks().size()) {
        return;
    }
    m_subtitleCues.clear();
    m_subtitleFileName = index >= 0 && index < m_subtitleTracks.size()
                             ? m_subtitleTracks.at(index).toMap()
                                   .value(QStringLiteral("title")).toString()
                             : QString{};
    m_player->setActiveSubtitleTrack(index);
    updateSubtitleText(position());
    emit subtitleFileChanged();
    emit tracksChanged();
}

void PlayerController::addToPlaylist(const QUrl &url)
{
    if (!url.isValid() || url.isEmpty()) {
        return;
    }
    const QString urlString = StudioController::publicUrl(url).toString();
    for (const QVariant &item : std::as_const(m_playlist)) {
        if (item.toMap().value(QStringLiteral("url")).toString() == urlString) {
            return;
        }
    }
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.fileName();
    QVariantMap entry;
    entry.insert(QStringLiteral("url"), urlString);
    entry.insert(QStringLiteral("title"), QFileInfo(path).fileName());
    entry.insert(QStringLiteral("kind"), isImageFile(path) ? QStringLiteral("Image")
                                                            : QStringLiteral("Media"));
    m_playlist.push_back(entry);
    savePlaylist();
    emit playlistChanged();
}

void PlayerController::removeFromPlaylist(int index)
{
    if (index < 0 || index >= m_playlist.size()) {
        return;
    }
    m_playlist.removeAt(index);
    if (m_playlistIndex == index) {
        m_playlistIndex = -1;
    } else if (m_playlistIndex > index) {
        --m_playlistIndex;
    }
    savePlaylist();
    emit playlistChanged();
}

void PlayerController::clearPlaylist()
{
    m_playlist.clear();
    m_playlistIndex = -1;
    savePlaylist();
    emit playlistChanged();
}

void PlayerController::playPlaylistIndex(int index)
{
    if (index < 0 || index >= m_playlist.size()) {
        return;
    }
    m_playlistIndex = index;
    emit playlistChanged();
    openMedia(QUrl(m_playlist.at(index).toMap().value(QStringLiteral("url")).toString()));
}

void PlayerController::playNextPlaylistItem()
{
    if (m_playlist.isEmpty()) {
        return;
    }
    playPlaylistIndex(m_playlistIndex >= 0 ? (m_playlistIndex + 1) % m_playlist.size() : 0);
}

void PlayerController::playPreviousPlaylistItem()
{
    if (m_playlist.isEmpty()) {
        return;
    }
    playPlaylistIndex(m_playlistIndex > 0 ? m_playlistIndex - 1 : m_playlist.size() - 1);
}

void PlayerController::openRecent(int index)
{
    if (index < 0 || index >= m_recentMedia.size()) {
        return;
    }
    openMedia(QUrl(m_recentMedia.at(index).toMap().value(QStringLiteral("url")).toString()));
}

void PlayerController::clearRecentMedia()
{
    m_recentMedia.clear();
    saveRecentMedia();
    emit recentMediaChanged();
}

void PlayerController::setThemeName(const QString &value)
{
    if (value.isEmpty() || m_themeName == value) {
        return;
    }
    m_themeName = value;
    QSettings().setValue(QStringLiteral("appearance/theme"), value);
    emit themeNameChanged();
}

void PlayerController::saveWindowState(int x, int y, int width, int height, bool maximized)
{
    QSettings settings;
    if (!maximized && width >= 980 && height >= 680) {
        m_savedWindowX = x;
        m_savedWindowY = y;
        m_savedWindowWidth = width;
        m_savedWindowHeight = height;
        settings.setValue(QStringLiteral("window/x"), x);
        settings.setValue(QStringLiteral("window/y"), y);
        settings.setValue(QStringLiteral("window/width"), width);
        settings.setValue(QStringLiteral("window/height"), height);
    }
    m_savedWindowMaximized = maximized;
    settings.setValue(QStringLiteral("window/maximized"), maximized);
}

void PlayerController::captureFrameInternal(bool applyFilters)
{
    const QImage frame=m_imageMode?m_originalImage:m_currentFrame.toImage();
    if(frame.isNull()){emit notification("Frame not ready","Open a video or image and wait for its first frame.");return;}
    runCapture(frame,m_imageMode?0:qMax<qint64>(0,m_currentFrame.startTime()/1000),applyFilters);
}



void PlayerController::revealLastCapture()
{
    if (m_lastCapturePath.isEmpty()) {
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_lastCapturePath).absolutePath()));
}

void PlayerController::enterCompareMode()
{
    if (!m_hasMedia) {
        emit notification(QStringLiteral("Nothing to compare"),
                          QStringLiteral("Open a video or image first."));
        return;
    }
    if (!m_imageMode && playing()) {
        m_player->pause();
    }
    m_compareModeActive = true;
    // Emitted unconditionally (even before the frame grab below) so the
    // overlay always appears immediately; if the video's first frame is not
    // decoded yet, updateCompareImages() below is a no-op and the
    // videoFrameChanged hook in attachVideoOutput() fills it in as soon as
    // the frame arrives.
    emit compareModeChanged();
    updateCompareImages();
}

void PlayerController::exitCompareMode()
{
    if (!m_compareModeActive) {
        return;
    }
    m_compareModeActive = false;
    emit compareModeChanged();
}

void PlayerController::updateCompareImages()
{
    if(m_compareBusy){m_comparePending=true;return;}
    const auto frame=m_imageMode?m_originalImage:m_currentFrame.toImage();
    if(frame.isNull())return;
    m_compareOriginalImage=frame;m_compareTimestampMs=m_imageMode?0:qMax<qint64>(0,m_currentFrame.startTime()/1000);
    m_compareOriginalSource=FrameStore::put("original",frame);
    m_compareFilteredSource=m_compareOriginalSource;
    const auto generation=m_generation, revision=++m_compareRevision;
    emit compareModeChanged();
    m_compareBusy=true;
    m_capturePipeline->process(frame,m_compareTimestampMs*1000,m_plugins->snapshot(),[this,generation,revision](QImage result,QStringList errors){
        m_compareBusy=false;
        const bool pending=m_comparePending;m_comparePending=false;
        if(generation!=m_generation||revision!=m_compareRevision||!m_compareModeActive){
            if(m_compareModeActive)updateCompareImages();return;
        }
        m_compareFilteredSource=FrameStore::put("comparison",result);m_filterError=errors.join("; ");
        emit compareModeChanged();emit diagnosticsChanged();
        if(pending)updateCompareImages();
    });
}

void PlayerController::refreshCompareFrame()
{
    if (m_compareModeActive) {
        updateCompareImages();
    }
}

void PlayerController::exportCompareFrame(bool filtered)
{
    if(m_compareOriginalImage.isNull())return;
    runCapture(m_compareOriginalImage,m_compareTimestampMs,filtered);
}

void PlayerController::clear()
{
    saveResumePosition();
    ++m_generation;m_stepFreeze=false;m_stepDirection=0;m_currentFrame={};clearLoop();
    m_player->stop();
    m_player->setSource(QUrl{});
    m_source = {};
    m_imageSource = {};
    m_originalImage = {};
    if (m_compareModeActive) {
        m_compareModeActive = false;
        m_compareOriginalImage = {};
        m_compareOriginalSource = {};
        m_compareFilteredSource = {};
        emit compareModeChanged();
    }
    clearSubtitles();
    m_title.clear();
    m_mediaKind = QStringLiteral("No media");
    m_hasMedia = false;
    m_imageMode = false;
    m_isLiveStream = false;
    m_videoWidth = 0;
    m_videoHeight = 0;
    setStatusText(QStringLiteral("Ready"));
    emit mediaChanged();
    emit playingChanged();
    emit positionChanged();
    emit durationChanged();
    emit videoSizeChanged();
}

void PlayerController::setStatusText(const QString &status)
{
    if (m_statusText == status) {
        return;
    }
    m_statusText = status;
    emit statusTextChanged();
}

void PlayerController::updateVideoSize()
{
    if (!m_videoSink) {
        return;
    }
    const QSize size = m_videoSink->videoSize();
    if (size.width() == m_videoWidth && size.height() == m_videoHeight) {
        return;
    }
    m_videoWidth = size.width();
    m_videoHeight = size.height();
    emit videoSizeChanged();
}

void PlayerController::refreshAudioDevice()
{
    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {
        if (m_audioDeviceName != QStringLiteral("No audio output")) {
            m_audioDeviceName = QStringLiteral("No audio output");
            emit audioDeviceChanged();
        }
        return;
    }

    if (m_audioDeviceName != device.description()) {
        m_audioDeviceName = device.description();
        emit audioDeviceChanged();
    }

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setSampleFormat(QAudioFormat::Int16);
    if (!device.isFormatSupported(format)) {
        format = device.preferredFormat();
    }

    m_player->setAudioBufferOutput(nullptr);
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
        m_audioSinkDevice = nullptr;
    }
    delete m_audioBufferOutput;

    m_audioBufferOutput = new QAudioBufferOutput(format, this);
    connect(m_audioBufferOutput, &QAudioBufferOutput::audioBufferReceived,
            this, &PlayerController::handleAudioBuffer);
    m_player->setAudioBufferOutput(m_audioBufferOutput);

    m_audioSink = new QAudioSink(device, format, this);
    m_audioSink->setBufferSize(format.bytesForDuration(500000));
    m_audioSink->setVolume(sinkVolume(m_volume, m_muted));
    connect(m_audioSink, &QAudioSink::stateChanged, this, [this](QAudio::State state) {
        if (state == QAudio::StoppedState && m_audioSink &&
            m_audioSink->error() != QAudio::NoError && !m_audioErrorReported) {
            m_audioErrorReported = true;
            emit notification(QStringLiteral("Audio output error"),
                              QStringLiteral("The selected audio device stopped."));
        }
    });
    m_audioErrorReported = false;
    m_audioSinkDevice = m_audioSink->start();
}

void PlayerController::restartAudioSink()
{
    if (!m_audioSink) {
        refreshAudioDevice();
        return;
    }
    m_audioSink->reset();
    m_audioErrorReported = false;
    m_audioSinkDevice = m_audioSink->start();
}

void PlayerController::handleAudioBuffer(const QAudioBuffer &buffer)
{
    if (!buffer.isValid() || !m_audioSinkDevice) {
        return;
    }

    const char *audioData = buffer.constData<char>();
    QByteArray boosted;
    if (!m_muted && m_volume > 1.0) {
        boosted = QByteArray(audioData, buffer.byteCount());
        const double gain = m_volume;
        switch (buffer.format().sampleFormat()) {
        case QAudioFormat::Int16: {
            auto *samples = reinterpret_cast<qint16 *>(boosted.data());
            const qsizetype count = boosted.size() / static_cast<qsizetype>(sizeof(qint16));
            for (qsizetype index = 0; index < count; ++index) {
                const double value = static_cast<double>(samples[index]) * gain;
                samples[index] = static_cast<qint16>(std::clamp(
                    value, static_cast<double>(std::numeric_limits<qint16>::min()),
                    static_cast<double>(std::numeric_limits<qint16>::max())));
            }
            break;
        }
        case QAudioFormat::Int32: {
            auto *samples = reinterpret_cast<qint32 *>(boosted.data());
            const qsizetype count = boosted.size() / static_cast<qsizetype>(sizeof(qint32));
            for (qsizetype index = 0; index < count; ++index) {
                const double value = static_cast<double>(samples[index]) * gain;
                samples[index] = static_cast<qint32>(std::clamp(
                    value, static_cast<double>(std::numeric_limits<qint32>::min()),
                    static_cast<double>(std::numeric_limits<qint32>::max())));
            }
            break;
        }
        case QAudioFormat::Float: {
            auto *samples = reinterpret_cast<float *>(boosted.data());
            const qsizetype count = boosted.size() / static_cast<qsizetype>(sizeof(float));
            for (qsizetype index = 0; index < count; ++index) {
                samples[index] = std::clamp(static_cast<float>(samples[index] * gain), -1.0F, 1.0F);
            }
            break;
        }
        case QAudioFormat::UInt8: {
            auto *samples = reinterpret_cast<quint8 *>(boosted.data());
            for (qsizetype index = 0; index < boosted.size(); ++index) {
                const double centered = (static_cast<double>(samples[index]) - 128.0) * gain + 128.0;
                samples[index] = static_cast<quint8>(std::clamp(centered, 0.0, 255.0));
            }
            break;
        }
        default:
            break;
        }
        audioData = boosted.constData();
    }

    const qint64 written = m_audioSinkDevice->write(
        audioData, static_cast<qint64>(buffer.byteCount()));
    if (written < 0 && !m_audioErrorReported) {
        m_audioErrorReported = true;
        emit notification(QStringLiteral("Audio output error"),
                          QStringLiteral("Andiya could not write decoded audio to the output device."));
    }
}

void PlayerController::refreshImagePreview()
{
    if(!m_hasMedia||!m_imageMode||m_originalImage.isNull())return;
    if(m_imageBusy){m_imagePending=true;return;}m_imageBusy=true;
    const auto generation=m_generation,revision=++m_previewRevision;
    m_previewPipeline->process(m_originalImage,0,m_plugins->snapshot(),[this,generation,revision](QImage result,QStringList errors){
        m_imageBusy=false;const bool pending=m_imagePending;m_imagePending=false;
        if(generation!=m_generation||revision!=m_previewRevision){refreshImagePreview();return;}
        m_imageSource=FrameStore::put("image",result);m_filterError=errors.join("; ");
        emit mediaChanged();emit diagnosticsChanged();
        if(pending)refreshImagePreview();
    });
}

QString PlayerController::captureDirectory() const
{
    if (!m_captureDirectory.isEmpty()) {
        return m_captureDirectory;
    }
    QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (pictures.isEmpty()) {
        pictures = QDir::homePath();
    }
    return QDir(pictures).filePath(QStringLiteral("Andiya Captures"));
}

void PlayerController::updateSubtitleText(qint64 positionMs)
{
    QString current = m_subtitleCues.isEmpty() ? m_embeddedSubtitleText : QString{};
    if (!m_subtitleCues.isEmpty()) {
        const qint64 adjustedPosition = positionMs - m_subtitleDelayMs;
        for (const SubtitleCue &cue : std::as_const(m_subtitleCues)) {
            if (adjustedPosition < cue.startMs) {
                break;
            }
            if (adjustedPosition <= cue.endMs) {
                current = cue.text;
                break;
            }
        }
    }
    if (current != m_subtitleText) {
        m_subtitleText = current;
        emit subtitleTextChanged();
    }
}

void PlayerController::updateTracks()
{
    const auto makeTracks = [](const QList<QMediaMetaData> &tracks, const QString &fallback) {
        QVariantList result;
        for (int index = 0; index < tracks.size(); ++index) {
            const QMediaMetaData &metadata = tracks.at(index);
            QString title = metadata.stringValue(QMediaMetaData::Title);
            const QString language = metadata.stringValue(QMediaMetaData::Language);
            if (title.isEmpty()) {
                title = QStringLiteral("%1 %2").arg(fallback).arg(index + 1);
            }
            if (!language.isEmpty() && !title.contains(language, Qt::CaseInsensitive)) {
                title += QStringLiteral(" - ") + language;
            }
            result.push_back(QVariantMap{{QStringLiteral("index"), index},
                                         {QStringLiteral("title"), title}});
        }
        return result;
    };
    m_audioTracks = makeTracks(m_player->audioTracks(), QStringLiteral("Audio"));
    m_subtitleTracks = makeTracks(m_player->subtitleTracks(), QStringLiteral("Subtitle"));
    emit tracksChanged();
}

void PlayerController::addRecentMedia(const QUrl &url)
{
    const QString urlString = StudioController::publicUrl(url).toString();
    for (int index = m_recentMedia.size() - 1; index >= 0; --index) {
        if (m_recentMedia.at(index).toMap().value(QStringLiteral("url")).toString() == urlString) {
            m_recentMedia.removeAt(index);
        }
    }
    QVariantMap entry;
    entry.insert(QStringLiteral("url"), urlString);
    entry.insert(QStringLiteral("title"), m_title);
    entry.insert(QStringLiteral("kind"), m_mediaKind);
    entry.insert(QStringLiteral("live"), m_isLiveStream);
    entry.insert(QStringLiteral("opened"),
                 QDateTime::currentDateTime().toString(QStringLiteral("MMM d, hh:mm")));
    entry.insert(QStringLiteral("position"), savedResumePosition(url));
    m_recentMedia.prepend(entry);
    while (m_recentMedia.size() > 24) {
        m_recentMedia.removeLast();
    }
    saveRecentMedia();
    emit recentMediaChanged();
}

void PlayerController::loadPersistentState()
{
    QSettings settings;
    m_captureDirectory = settings.value(QStringLiteral("capture/directory")).toString();
    m_recentMedia = settings.value(QStringLiteral("library/recent")).toList();
    m_bookmarks = settings.value(QStringLiteral("library/bookmarks")).toList();
    m_playlist = settings.value(QStringLiteral("library/playlist")).toList();
    for(auto *items:{&m_recentMedia,&m_playlist,&m_bookmarks})for(auto &value:*items){
        auto item=value.toMap();const QUrl url(item.value("url").toString());
        if(!url.isLocalFile()){
            const auto safe=StudioController::publicUrl(url);item["url"]=safe.toString();
            if(item.contains("title"))item["title"]=safe.fileName().isEmpty()?safe.host():safe.fileName();
        }value=item;
    }
    saveRecentMedia();savePlaylist();settings.setValue("library/bookmarks",m_bookmarks);
    m_themeName = settings.value(QStringLiteral("appearance/theme"),
                                 QStringLiteral("Aurora Glass")).toString();
    m_subtitleDelayMs = settings.value(QStringLiteral("subtitles/delayMs"), 0).toInt();
    m_subtitleFontSize = settings.value(QStringLiteral("subtitles/fontSize"), 18).toInt();
    m_subtitleColor = settings.value(QStringLiteral("subtitles/color"),
                                     QStringLiteral("#FFFFFF")).toString();
    m_subtitlePosition = settings.value(QStringLiteral("subtitles/position"), 8).toInt();
    m_savedWindowX = settings.value(QStringLiteral("window/x"), -1).toInt();
    m_savedWindowY = settings.value(QStringLiteral("window/y"), -1).toInt();
    m_savedWindowWidth = settings.value(QStringLiteral("window/width"), 1440).toInt();
    m_savedWindowHeight = settings.value(QStringLiteral("window/height"), 900).toInt();
    m_savedWindowMaximized = settings.value(QStringLiteral("window/maximized"), false).toBool();
    m_player->setPlaybackRate(settings.value(QStringLiteral("playback/rate"), 1.0).toReal());
}

void PlayerController::saveRecentMedia() const
{
    QSettings().setValue(QStringLiteral("library/recent"), m_recentMedia);
}

void PlayerController::savePlaylist() const
{
    QSettings().setValue(QStringLiteral("library/playlist"), m_playlist);
}

QString PlayerController::resumeKey(const QUrl &url) const
{
    return QString::fromLatin1(QCryptographicHash::hash(url.toString().toUtf8(),
                                                        QCryptographicHash::Sha256).toHex());
}

qint64 PlayerController::savedResumePosition(const QUrl &url) const
{
    if (!url.isValid() || url.isEmpty()) {
        return 0;
    }
    QSettings settings;
    settings.beginGroup(QStringLiteral("resume"));
    return settings.value(resumeKey(url), 0).toLongLong();
}

void PlayerController::saveResumePosition()
{
    if (!m_hasMedia || m_imageMode || m_isLiveStream || !seekable() || !m_source.isValid()) {
        return;
    }
    const qint64 currentPosition = position();
    QSettings settings;
    settings.beginGroup(QStringLiteral("resume"));
    const QString key = resumeKey(m_source);
    if (currentPosition < 2'000 ||
        (duration() > 0 && currentPosition >= duration() * 95 / 100)) {
        settings.remove(key);
    } else {
        settings.setValue(key, currentPosition);
    }
    settings.endGroup();

    for (int index = 0; index < m_recentMedia.size(); ++index) {
        QVariantMap item = m_recentMedia.at(index).toMap();
        if (item.value(QStringLiteral("url")).toString() == m_source.toString()) {
            item.insert(QStringLiteral("position"), currentPosition);
            m_recentMedia[index] = item;
            saveRecentMedia();
            emit recentMediaChanged();
            break;
        }
    }
}

void PlayerController::autoLoadSubtitle(const QString &mediaPath)
{
    const QFileInfo media(mediaPath);
    const QString base = media.dir().filePath(media.completeBaseName());
    const QStringList extensions = {QStringLiteral(".srt"), QStringLiteral(".vtt")};
    for (const QString &extension : extensions) {
        const QString candidate = base + extension;
        if (QFileInfo::exists(candidate)) {
            openSubtitle(QUrl::fromLocalFile(candidate));
            return;
        }
    }
}



bool PlayerController::isImageFile(const QString &path)
{
    static const QStringList extensions = {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("bmp"), QStringLiteral("gif"), QStringLiteral("webp"),
        QStringLiteral("tif"), QStringLiteral("tiff"), QStringLiteral("avif")
    };
    return extensions.contains(QFileInfo(path).suffix().toLower());
}

QString PlayerController::backendName() const { return QStringLiteral("Qt Multimedia"); }
bool PlayerController::seekable() const { return m_player->isSeekable(); }
QVariantList PlayerController::bookmarks() const { return m_bookmarks; }
void PlayerController::addBookmark(const QString &note) { if (!m_hasMedia || m_imageMode || m_bookmarks.size()>=200 || note.size()>4096) return; m_bookmarks.push_back(QVariantMap{{"url", StudioController::publicUrl(m_source).toString()}, {"position", position()}, {"label", note.trimmed().isEmpty() ? QStringLiteral("Bookmark") : note}}); QSettings().setValue(QStringLiteral("library/bookmarks"), m_bookmarks); emit bookmarksChanged(); }
void PlayerController::removeBookmark(int index) { if(index>=0 && index<m_bookmarks.size()){m_bookmarks.removeAt(index); QSettings().setValue(QStringLiteral("library/bookmarks"), m_bookmarks); emit bookmarksChanged();} }
void PlayerController::jumpBookmark(int index) {
    if(index<0||index>=m_bookmarks.size())return;
    const auto item=m_bookmarks[index].toMap();const QUrl url(item.value("url").toString());
    const auto target=item.value("position").toLongLong();
    if(url!=m_source){openMedia(url);m_pendingResumePosition=target;}else seek(target);
}
void PlayerController::setLoopStart() { if(!seekable()||m_imageMode)return; m_loopStart=position(); if(m_loopEnd>=0&&m_loopEnd<=m_loopStart)m_loopEnd=-1; m_loopEnabled=false; emit loopChanged(); }
void PlayerController::setLoopEnd() { if(!seekable()||m_imageMode)return; m_loopEnd=position(); if(m_loopStart>=0&&m_loopEnd>m_loopStart)m_loopEnabled=true; emit loopChanged(); }
void PlayerController::clearLoop() { m_loopStart=m_loopEnd=-1; m_loopEnabled=false; emit loopChanged(); }

void PlayerController::reconnectStream() {const auto url=m_source;if(!url.isEmpty()){m_player->setSource({});openMedia(url);}}
void PlayerController::setLiveFilters(bool enabled) {m_liveFilters=enabled;QSettings().setValue("filters/live",enabled);++m_generation;requestPreview();emit streamSettingsChanged();}
void PlayerController::startBatchCapture(int intervalSeconds,int count,bool filtered,bool contactSheet) {
    if(m_exports->running())return;
    if(!m_hasMedia||m_imageMode||!seekable()){emit notification("Export unavailable","Load a seekable video first.");return;}
    QVariantList items;const auto start=position();const qint64 interval=qBound(1,intervalSeconds,86400)*1000LL;
    for(int i=0;i<qBound(1,count,200);++i) {
        const auto target=start+i*interval;if(target>=duration())break;
        items<<QVariantMap{{"url",m_source.toString()},{"position",target}};
    }
    if(items.isEmpty())return;
    m_batchRunning=true;m_batchProgress=0;emit batchChanged();
    m_exports->start(items,captureDirectory(),filtered?m_plugins->snapshot():QVariantList{},contactSheet);
}
void PlayerController::exportBookmarks(bool filtered,bool contactSheet) {
    if(m_exports->running())return;
    if(m_bookmarks.isEmpty()){emit notification("No bookmarks","Add bookmarks before exporting.");return;}
    m_batchRunning=true;m_batchProgress=0;emit batchChanged();
    m_exports->start(m_bookmarks.mid(0,200),captureDirectory(),filtered?m_plugins->snapshot():QVariantList{},contactSheet);
}
void PlayerController::exportImages(const QList<QUrl> &urls) {
    if(m_exports->running()||urls.isEmpty())return;
    QVariantList items;for(const auto &url:urls.mid(0,200)) {
        if(!url.isLocalFile())continue;
        items<<QVariantMap{{"url",url.toString()},{"image",true},{"position",0}};
    }
    if(items.isEmpty())return;
    m_batchRunning=true;m_batchProgress=0;emit batchChanged();
    m_exports->start(items,captureDirectory(),m_plugins->snapshot(),false);
}
void PlayerController::cancelBatch() {m_exports->cancel();}

void PlayerController::handleDecodedFrame(const QVideoFrame &frame) {
    if(!frame.isValid()||m_imageMode)return;
    ++m_decodedFrames;
    if(m_stepFreeze)return;
    if(m_stepDirection) {
        const auto ts=frame.startTime();
        if(m_stepDirection<0) {
            if(ts<m_stepOriginUs){m_previousCandidate=frame;return;}
            if(!m_previousCandidate.isValid())return;
            m_currentFrame=m_previousCandidate;
        } else {
            if(ts<=m_stepOriginUs)return;
            m_currentFrame=frame;
        }
        m_stepDirection=0;m_stepFreeze=true;m_player->pause();
        m_player->setPosition(qMax<qint64>(0,m_currentFrame.startTime()/1000));
        emit positionChanged();updateSubtitleText(position());
    } else m_currentFrame=frame;
    if(!m_liveFilters||!m_plugins->hasEnabledImageFilters()||!m_displaySink||!m_displaySink->videoFrame().isValid())presentFrame(m_currentFrame);
    requestPreview();
    if(m_compareModeActive&&!playing())updateCompareImages();
}
void PlayerController::presentFrame(const QVideoFrame &frame) {
    if(m_displaySink)m_displaySink->setVideoFrame(frame);
}
void PlayerController::requestPreview() {
    if(m_imageMode||!m_currentFrame.isValid())return;
    if(!m_liveFilters||!m_plugins->hasEnabledImageFilters()){presentFrame(m_currentFrame);return;}
    if(m_previewBusy){++m_skippedPreviews;return;}
    m_previewBusy=true;
    const auto frame=m_currentFrame;const auto generation=m_generation;
    auto image=frame.toImage();
    if(image.width()>960 || image.height()>960)image=image.scaled(960,960,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    m_previewPipeline->process(image,frame.startTime(),m_plugins->snapshot(),[this,generation,frame](QImage result,QStringList errors){
        m_previewBusy=false;
        if(generation!=m_generation){requestPreview();return;}
        m_filterError=errors.join("; ");
        QVideoFrame filtered(result);filtered.setStartTime(frame.startTime());filtered.setEndTime(frame.endTime());
        presentFrame(filtered);emit diagnosticsChanged();
    });
}
void PlayerController::runCapture(QImage frame,qint64 timestamp,bool filtered) {
    if(m_pendingCaptures>=4){emit notification("Capture queue full","Wait for an export to finish.");return;}
    ++m_pendingCaptures;
    const QString directory=captureDirectory();
    const QString path=QDir(directory).filePath(QString("frame_%1ms_%2_%3.png").arg(timestamp).arg(filtered?"filtered":"original",QUuid::createUuid().toString(QUuid::WithoutBraces)));
    m_capturePipeline->process(frame,timestamp*1000,filtered?m_plugins->snapshot():QVariantList{},[this,path,directory](QImage result,QStringList errors){
        if(!errors.isEmpty()){--m_pendingCaptures;emit notification("Capture stopped",errors.join("; "));return;}
        QPointer<PlayerController> guard(this);
        QThreadPool::globalInstance()->start([guard,result,path,directory]{
            QDir().mkpath(directory);QSaveFile file(path);
            const bool ok=file.open(QIODevice::WriteOnly)&&result.save(&file,"PNG")&&file.commit();
            if(guard)QMetaObject::invokeMethod(guard,[guard,path,ok]{
                if(!guard)return;
                --guard->m_pendingCaptures;
                if(ok){guard->m_lastCapturePath=path;emit guard->lastCapturePathChanged();}
                emit guard->notification(ok?"Frame captured":"Capture failed",ok?path:"Check the export folder and free disk space.");
            });
        });
    });
}
QVariantMap PlayerController::diagnostics() const {
    const auto metadata=m_player->metaData();
    return {{"Backend",backendName()},{"Video codec",metadata.stringValue(QMediaMetaData::VideoCodec)},
            {"Audio codec",metadata.stringValue(QMediaMetaData::AudioCodec)},
            {"Container",metadata.stringValue(QMediaMetaData::FileFormat)},
            {"Frame rate",metadata.value(QMediaMetaData::VideoFrameRate)},
            {"Resolution",QString("%1 × %2").arg(m_videoWidth).arg(m_videoHeight)},
            {"Decoder",QString("Selected automatically by Qt; hardware decoder details are not exposed")},
            {"Buffer",QString::number(qRound(m_player->bufferProgress()*100))+"%"},
            {"Seekable",seekable()?"Yes":"No"},{"Decoded frames received",QVariant::fromValue(m_decodedFrames)},
            {"Filter previews skipped",QVariant::fromValue(m_skippedPreviews)},
            {"Audio output",m_audioDeviceName},{"Filters",m_filterError.isEmpty()?"Healthy":m_filterError},
            {"Status",m_statusText}};
}
void PlayerController::editBookmark(int index,const QString &label) {
    if(index<0||index>=m_bookmarks.size()||label.size()>4096)return;
    auto item=m_bookmarks[index].toMap();item["label"]=label;m_bookmarks[index]=item;
    QSettings().setValue("library/bookmarks",m_bookmarks);emit bookmarksChanged();
}
QVariantMap PlayerController::workspaceData() const {
    return {{"bookmarks",m_bookmarks},{"playlist",m_playlist},{"theme",m_themeName},
            {"presets",QSettings().value("plugins/presets").toMap()}};
}
bool PlayerController::restoreWorkspace(const QVariantMap &data,QString *error) {
    const auto bookmarks=data.value("bookmarks").toList(),playlist=data.value("playlist").toList();
    if(bookmarks.size()>200||playlist.size()>2000){*error="Workspace has too many items.";return false;}
    auto valid=[](const QVariantList &items,bool timed){
        for(const auto &value:items) {
            const auto item=value.toMap();const QUrl url(item.value("url").toString());
            if(!url.isValid()||url.isEmpty()||url.isRelative()||item.value("label").toString().size()>4096)return false;
            if(timed && (item.value("position").toLongLong()<0||item.value("position").toLongLong()>7*24*3600000LL))return false;
        }
        return true;
    };
    if(!valid(bookmarks,true)||!valid(playlist,false)){*error="Workspace contains invalid media entries.";return false;}
    m_bookmarks=bookmarks;m_playlist=playlist;m_playlistIndex=-1;
    for(auto *items:{&m_bookmarks,&m_playlist})for(auto &value:*items) {
        auto item=value.toMap();item["url"]=StudioController::publicUrl(QUrl(item.value("url").toString())).toString();value=item;
    }
    QSettings settings;settings.setValue("library/bookmarks",m_bookmarks);savePlaylist();
    settings.setValue("plugins/presets",data.value("presets").toMap());
    emit bookmarksChanged();emit playlistChanged();m_plugins->reload();emit m_plugins->presetsChanged();
    setThemeName(data.value("theme","Aurora Glass").toString());return true;
}
