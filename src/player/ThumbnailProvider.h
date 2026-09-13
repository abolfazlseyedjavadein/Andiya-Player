#pragma once

#include <QObject>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QUrl>

class QMediaPlayer;
class QTimer;
class QVideoFrame;
class QVideoSink;

// Generates small preview images for video files by decoding a single frame
// a few seconds in, then caching the result to disk so the cost is paid once
// per file. Image files need no decoding and are not handled here; callers
// should use the file itself as its own thumbnail.
class ThumbnailProvider final : public QObject
{
    Q_OBJECT

public:
    explicit ThumbnailProvider(QObject *parent = nullptr);
    ~ThumbnailProvider() override;

    // Returns a QML-usable "file:///..." URL string if a thumbnail already
    // exists on disk for this source, otherwise an empty string.
    Q_INVOKABLE QString cachedThumbnailUrl(const QUrl &sourceUrl) const;

    // Enqueues background generation if no cached thumbnail exists yet.
    // Jobs run one at a time; completion is reported via thumbnailReady().
    Q_INVOKABLE void requestThumbnail(const QUrl &sourceUrl);

signals:
    void thumbnailReady(const QUrl &sourceUrl, const QString &thumbnailUrl);

private:
    void processNext();
    void handleMediaStatusChanged(int status);
    void handleFrameChanged(const QVideoFrame &frame);
    void finishActiveJob(const QString &savedPath);
    void failActiveJob();
    [[nodiscard]] QString cacheFilePath(const QUrl &sourceUrl) const;

    QMediaPlayer *m_player = nullptr;
    QVideoSink *m_sink = nullptr;
    QTimer *m_captureTimeout = nullptr;
    QQueue<QUrl> m_queue;
    QSet<QString> m_seen;
    QUrl m_activeUrl;
    bool m_startQueued = false;
    bool m_frameCaptured = false;
};
