#pragma once

#include <QObject>
#include <QImage>
#include <QPointer>
#include <QUrl>
#include <QVariantList>
#include <QVector>
#include <QVideoFrame>

class QAudioBuffer;
class QAudioBufferOutput;
class QAudioSink;
class QIODevice;
class QMediaPlayer;
class QTimer;
class QVideoSink;
class PluginListModel;
class FilterPipeline;
class ExportQueue;

class PlayerController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString backendName READ backendName NOTIFY mediaChanged)
    Q_PROPERTY(bool seekable READ seekable NOTIFY durationChanged)
    Q_PROPERTY(QVariantList bookmarks READ bookmarks NOTIFY bookmarksChanged)
    Q_PROPERTY(qint64 loopStart MEMBER m_loopStart NOTIFY loopChanged)
    Q_PROPERTY(qint64 loopEnd MEMBER m_loopEnd NOTIFY loopChanged)
    Q_PROPERTY(bool loopEnabled MEMBER m_loopEnabled NOTIFY loopChanged)
    Q_PROPERTY(bool batchRunning MEMBER m_batchRunning NOTIFY batchChanged)
    Q_PROPERTY(QString batchStatus MEMBER m_batchStatus NOTIFY batchChanged)
    Q_PROPERTY(double batchProgress MEMBER m_batchProgress NOTIFY batchChanged)
    Q_PROPERTY(bool liveFilters MEMBER m_liveFilters NOTIFY streamSettingsChanged)


    Q_PROPERTY(QString title READ title NOTIFY mediaChanged)
    Q_PROPERTY(QString mediaKind READ mediaKind NOTIFY mediaChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QUrl source READ source NOTIFY mediaChanged)
    Q_PROPERTY(QUrl imageSource READ imageSource NOTIFY mediaChanged)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY mediaChanged)
    Q_PROPERTY(bool imageMode READ imageMode NOTIFY mediaChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY volumeChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(QString audioDeviceName READ audioDeviceName NOTIFY audioDeviceChanged)
    Q_PROPERTY(QString captureDirectoryPath READ captureDirectoryPath NOTIFY captureDirectoryChanged)
    Q_PROPERTY(QString subtitleText READ subtitleText NOTIFY subtitleTextChanged)
    Q_PROPERTY(QString subtitleFileName READ subtitleFileName NOTIFY subtitleFileChanged)
    Q_PROPERTY(bool hasSubtitles READ hasSubtitles NOTIFY subtitleFileChanged)
    Q_PROPERTY(bool subtitleRtl READ subtitleRtl NOTIFY subtitleTextChanged)
    Q_PROPERTY(int subtitleDelayMs READ subtitleDelayMs WRITE setSubtitleDelayMs NOTIFY subtitleSettingsChanged)
    Q_PROPERTY(int subtitleFontSize READ subtitleFontSize WRITE setSubtitleFontSize NOTIFY subtitleSettingsChanged)
    Q_PROPERTY(QString subtitleColor READ subtitleColor WRITE setSubtitleColor NOTIFY subtitleSettingsChanged)
    Q_PROPERTY(int subtitlePosition READ subtitlePosition WRITE setSubtitlePosition NOTIFY subtitleSettingsChanged)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY tracksChanged)
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks NOTIFY tracksChanged)
    Q_PROPERTY(int activeAudioTrack READ activeAudioTrack NOTIFY tracksChanged)
    Q_PROPERTY(int activeSubtitleTrack READ activeSubtitleTrack NOTIFY tracksChanged)
    Q_PROPERTY(QVariantList recentMedia READ recentMedia NOTIFY recentMediaChanged)
    Q_PROPERTY(QVariantList playlist READ playlist NOTIFY playlistChanged)
    Q_PROPERTY(int playlistIndex READ playlistIndex NOTIFY playlistChanged)
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY themeNameChanged)
    Q_PROPERTY(int savedWindowX READ savedWindowX CONSTANT)
    Q_PROPERTY(int savedWindowY READ savedWindowY CONSTANT)
    Q_PROPERTY(int savedWindowWidth READ savedWindowWidth CONSTANT)
    Q_PROPERTY(int savedWindowHeight READ savedWindowHeight CONSTANT)
    Q_PROPERTY(bool savedWindowMaximized READ savedWindowMaximized CONSTANT)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int videoWidth READ videoWidth NOTIFY videoSizeChanged)
    Q_PROPERTY(int videoHeight READ videoHeight NOTIFY videoSizeChanged)
    Q_PROPERTY(QString lastCapturePath READ lastCapturePath NOTIFY lastCapturePathChanged)
    Q_PROPERTY(bool compareModeActive READ compareModeActive NOTIFY compareModeChanged)
    Q_PROPERTY(QUrl compareOriginalSource READ compareOriginalSource NOTIFY compareModeChanged)
    Q_PROPERTY(QUrl compareFilteredSource READ compareFilteredSource NOTIFY compareModeChanged)
    Q_PROPERTY(bool compareHasFilters READ compareHasFilters NOTIFY compareModeChanged)
    Q_PROPERTY(bool isLiveStream READ isLiveStream NOTIFY mediaChanged)

public:
    explicit PlayerController(PluginListModel *plugins, QObject *parent = nullptr);
    ~PlayerController() override;

    [[nodiscard]] QString title() const;
    [[nodiscard]] QString mediaKind() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QUrl source() const;
    [[nodiscard]] QUrl imageSource() const;
    [[nodiscard]] bool hasMedia() const;
    [[nodiscard]] bool imageMode() const;
    [[nodiscard]] bool playing() const;
    [[nodiscard]] bool muted() const;
    [[nodiscard]] qreal volume() const;
    [[nodiscard]] qreal playbackRate() const;
    [[nodiscard]] QString audioDeviceName() const;
    [[nodiscard]] QString captureDirectoryPath() const;
    [[nodiscard]] QString subtitleText() const;
    [[nodiscard]] QString subtitleFileName() const;
    [[nodiscard]] bool hasSubtitles() const;
    [[nodiscard]] bool subtitleRtl() const;
    [[nodiscard]] int subtitleDelayMs() const;
    [[nodiscard]] int subtitleFontSize() const;
    [[nodiscard]] QString subtitleColor() const;
    [[nodiscard]] int subtitlePosition() const;
    [[nodiscard]] QVariantList audioTracks() const;
    [[nodiscard]] QVariantList subtitleTracks() const;
    [[nodiscard]] int activeAudioTrack() const;
    [[nodiscard]] int activeSubtitleTrack() const;
    [[nodiscard]] QVariantList recentMedia() const;
    [[nodiscard]] QVariantList playlist() const;
    [[nodiscard]] int playlistIndex() const;
    [[nodiscard]] QString themeName() const;
    [[nodiscard]] int savedWindowX() const;
    [[nodiscard]] int savedWindowY() const;
    [[nodiscard]] int savedWindowWidth() const;
    [[nodiscard]] int savedWindowHeight() const;
    [[nodiscard]] bool savedWindowMaximized() const;
    [[nodiscard]] qint64 position() const;
    [[nodiscard]] qint64 duration() const;
    [[nodiscard]] int videoWidth() const;
    [[nodiscard]] int videoHeight() const;
    [[nodiscard]] QString lastCapturePath() const;
    [[nodiscard]] bool compareModeActive() const;
    [[nodiscard]] QUrl compareOriginalSource() const;
    [[nodiscard]] QUrl compareFilteredSource() const;
    [[nodiscard]] bool compareHasFilters() const;
    [[nodiscard]] bool isLiveStream() const;

    QString backendName() const;
    bool seekable() const;
    QVariantList bookmarks() const;
    Q_INVOKABLE void addBookmark(const QString &note);
    Q_INVOKABLE void removeBookmark(int index);
    Q_INVOKABLE void jumpBookmark(int index);
    Q_INVOKABLE void setLoopStart();
    Q_INVOKABLE void setLoopEnd();
    Q_INVOKABLE void clearLoop();
    Q_INVOKABLE void reconnectStream();
    Q_INVOKABLE void setLiveFilters(bool enabled);
    Q_INVOKABLE void startBatchCapture(int intervalSeconds, int count, bool filtered, bool contactSheet);
    Q_INVOKABLE void exportBookmarks(bool filtered, bool contactSheet);
    Q_INVOKABLE void exportImages(const QList<QUrl> &urls);
    Q_INVOKABLE void cancelBatch();

    Q_INVOKABLE void attachVideoOutput(QObject *videoOutput);
    Q_INVOKABLE void openMedia(const QUrl &url);
    // Opens a network URL typed by the user (RTSP/RTSPS/RTMP/HTTP(S)/etc.),
    // via the FFmpeg-based Qt Multimedia backend that ships with Andiya --
    // no plugin involved, since protocol/demuxing happens below the plugin
    // frame-filter layer. Validates the text into a proper QUrl first so a
    // malformed entry reports a friendly error instead of silently failing.
    Q_INVOKABLE void openNetworkStream(const QString &urlText);
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void jumpSeconds(int seconds);
    Q_INVOKABLE void stepFrame(int direction);
    Q_INVOKABLE void setVolume(qreal value);
    Q_INVOKABLE void setPlaybackRate(qreal value);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void captureFrame();
    Q_INVOKABLE void captureOriginalFrame();
    Q_INVOKABLE void revealCaptureDirectory();
    Q_INVOKABLE void revealLastCapture();
    // Pro-frame capture & compare: pauses (if needed), grabs the current
    // frame, and exposes both the untouched and filtered versions side by
    // side so the user can pick exactly which one to export.
    Q_INVOKABLE void enterCompareMode();
    Q_INVOKABLE void exitCompareMode();
    Q_INVOKABLE void exportCompareFrame(bool filtered);
    // Lets the UI (e.g. a "live preview" timer while playing) force a
    // refresh of both compare images on demand, bypassing the normal
    // paused-only guard used by the automatic videoFrameChanged hook.
    Q_INVOKABLE void refreshCompareFrame();
    Q_INVOKABLE void setCaptureDirectory(const QUrl &directoryUrl);
    Q_INVOKABLE void resetCaptureDirectory();
    Q_INVOKABLE void openSubtitle(const QUrl &url);
    Q_INVOKABLE void clearSubtitles();
    Q_INVOKABLE void setSubtitleDelayMs(int value);
    Q_INVOKABLE void setSubtitleFontSize(int value);
    Q_INVOKABLE void setSubtitleColor(const QString &value);
    Q_INVOKABLE void setSubtitlePosition(int value);
    Q_INVOKABLE void setActiveAudioTrack(int index);
    Q_INVOKABLE void setActiveSubtitleTrack(int index);
    Q_INVOKABLE void addToPlaylist(const QUrl &url);
    Q_INVOKABLE void removeFromPlaylist(int index);
    Q_INVOKABLE void clearPlaylist();
    Q_INVOKABLE void playPlaylistIndex(int index);
    Q_INVOKABLE void playNextPlaylistItem();
    Q_INVOKABLE void playPreviousPlaylistItem();
    Q_INVOKABLE void openRecent(int index);
    Q_INVOKABLE void clearRecentMedia();
    Q_INVOKABLE void setThemeName(const QString &value);
    Q_INVOKABLE void saveWindowState(int x, int y, int width, int height, bool maximized);
    Q_INVOKABLE void clear();

signals:
    void streamSettingsChanged();
    void bookmarksChanged();
    void loopChanged();
    void batchChanged();
    void mediaChanged();
    void playingChanged();
    void volumeChanged();
    void playbackRateChanged();
    void audioDeviceChanged();
    void positionChanged();
    void durationChanged();
    void videoSizeChanged();
    void statusTextChanged();
    void lastCapturePathChanged();
    void captureDirectoryChanged();
    void subtitleTextChanged();
    void subtitleFileChanged();
    void subtitleSettingsChanged();
    void tracksChanged();
    void recentMediaChanged();
    void playlistChanged();
    void themeNameChanged();
    void compareModeChanged();
    void notification(const QString &title, const QString &message);

private:
    void handleDecodedFrame(const QVideoFrame &frame);
    void presentFrame(const QVideoFrame &frame);
    void requestPreview();
    void runCapture(QImage frame, qint64 timestampMs, bool filtered);

    Q_PROPERTY(QVariantMap diagnostics READ diagnostics NOTIFY diagnosticsChanged)
public:
    QVariantMap diagnostics() const;
    Q_INVOKABLE void editBookmark(int index,const QString &label);
    Q_INVOKABLE QVariantMap workspaceData() const;
    bool restoreWorkspace(const QVariantMap &data,QString *error);
signals:
    void diagnosticsChanged();
private:
    FilterPipeline *m_previewPipeline=nullptr;
    FilterPipeline *m_capturePipeline=nullptr;
    ExportQueue *m_exports=nullptr;
    quint64 m_decodedFrames=0,m_skippedPreviews=0;
    QString m_filterError;

    QPointer<QVideoSink> m_displaySink;
    QVideoFrame m_currentFrame;
    QVideoFrame m_previousCandidate;
    bool m_previewBusy = false;
    bool m_compareBusy=false,m_comparePending=false,m_imageBusy=false,m_imagePending=false;
    bool m_liveFilters = true;
    quint64 m_generation = 0;
    int m_pendingCaptures = 0;
    int m_stepDirection = 0;
    qint64 m_stepOriginUs = -1;
    bool m_stepFreeze = false;
    qint64 m_compareTimestampMs = 0;
    QVariantList m_bookmarks;
    qint64 m_loopStart = -1;
    qint64 m_loopEnd = -1;
    bool m_loopEnabled = false;
    bool m_batchRunning = false;
    QString m_batchStatus;
    double m_batchProgress = 0;

    void setStatusText(const QString &status);
    void updateVideoSize();
    void refreshAudioDevice();
    void restartAudioSink();
    void handleAudioBuffer(const QAudioBuffer &buffer);
    void refreshImagePreview();
    void captureFrameInternal(bool applyFilters);
    void updateCompareImages();
    void updateSubtitleText(qint64 positionMs);
    void autoLoadSubtitle(const QString &mediaPath);
    void updateTracks();
    void addRecentMedia(const QUrl &url);
    void loadPersistentState();
    void saveRecentMedia() const;
    void savePlaylist() const;
    void saveResumePosition();
    [[nodiscard]] qint64 savedResumePosition(const QUrl &url) const;
    [[nodiscard]] QString resumeKey(const QUrl &url) const;
    [[nodiscard]] QString captureDirectory() const;
    [[nodiscard]] static bool isImageFile(const QString &path);

    QMediaPlayer *m_player = nullptr;
    QAudioBufferOutput *m_audioBufferOutput = nullptr;
    QAudioSink *m_audioSink = nullptr;
    QIODevice *m_audioSinkDevice = nullptr;
    QTimer *m_resumeTimer = nullptr;
    PluginListModel *m_plugins = nullptr;
    QPointer<QVideoSink> m_videoSink;

    QUrl m_source;
    QUrl m_imageSource;
    QString m_title;
    QString m_mediaKind = QStringLiteral("No media");
    QString m_statusText;
    QString m_lastCapturePath;
    QString m_audioDeviceName = QStringLiteral("No audio output");
    QImage m_originalImage;
    struct SubtitleCue {
        qint64 startMs = 0;
        qint64 endMs = 0;
        QString text;
    };
    QVector<SubtitleCue> m_subtitleCues;
    QString m_subtitleText;
    QString m_embeddedSubtitleText;
    QString m_subtitleFileName;
    QString m_captureDirectory;
    QVariantList m_audioTracks;
    QVariantList m_subtitleTracks;
    QVariantList m_recentMedia;
    QVariantList m_playlist;
    QString m_themeName = QStringLiteral("Aurora Glass");
    qint64 m_pendingResumePosition = 0;
    int m_playlistIndex = -1;
    int m_subtitleDelayMs = 0;
    int m_subtitleFontSize = 18;
    int m_subtitlePosition = 8;
    QString m_subtitleColor = QStringLiteral("#FFFFFF");
    int m_savedWindowX = -1;
    int m_savedWindowY = -1;
    int m_savedWindowWidth = 1440;
    int m_savedWindowHeight = 900;
    bool m_savedWindowMaximized = false;
    quint64 m_previewRevision = 0;
    quint64 m_compareRevision = 0;
    QImage m_compareOriginalImage;
    QUrl m_compareOriginalSource;
    QUrl m_compareFilteredSource;
    bool m_compareModeActive = false;
    qreal m_volume = 0.78;
    bool m_muted = false;
    bool m_audioErrorReported = false;
    bool m_hasMedia = false;
    bool m_imageMode = false;
    // True for URLs whose scheme is an inherently "live"/non-seekable
    // transport (rtsp, rtmp, mms, udp, rtp, ...). mediaKind stays "Video"/
    // "Audio" for these so existing VideoOutput visibility checks keep
    // working; UI that cares about live-only quirks (hide seek bar, skip
    // resume position, show a LIVE badge) reads this flag separately.
    bool m_isLiveStream = false;
    int m_videoWidth = 0;
    int m_videoHeight = 0;
};
