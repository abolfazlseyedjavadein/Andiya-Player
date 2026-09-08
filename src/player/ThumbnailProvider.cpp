#include "ThumbnailProvider.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QMediaPlayer>
#include <QStandardPaths>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

namespace {
constexpr qint64 kThumbnailSeekMs = 4000;
constexpr int kThumbnailMaxDimension = 320;
constexpr int kCaptureTimeoutMs = 7000;
}

ThumbnailProvider::ThumbnailProvider(QObject *parent)
    : QObject(parent),
      m_player(new QMediaPlayer(this)),
      m_sink(new QVideoSink(this)),
      m_captureTimeout(new QTimer(this))
{
    m_player->setVideoOutput(m_sink);
    m_captureTimeout->setSingleShot(true);
    m_captureTimeout->setInterval(kCaptureTimeoutMs);

    connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) { handleMediaStatusChanged(static_cast<int>(status)); });
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &) { failActiveJob(); });
    connect(m_sink, &QVideoSink::videoFrameChanged, this, &ThumbnailProvider::handleFrameChanged);
    connect(m_captureTimeout, &QTimer::timeout, this, &ThumbnailProvider::failActiveJob);
}

ThumbnailProvider::~ThumbnailProvider() = default;

QString ThumbnailProvider::cacheFilePath(const QUrl &sourceUrl) const
{
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                              + QStringLiteral("/thumbnails");
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(sourceUrl.toString().toUtf8(), QCryptographicHash::Sha256).toHex());
    return QDir(cacheDir).filePath(hash + QStringLiteral(".jpg"));
}

QString ThumbnailProvider::cachedThumbnailUrl(const QUrl &sourceUrl) const
{
    const QString path = cacheFilePath(sourceUrl);
    return QFileInfo::exists(path) ? QUrl::fromLocalFile(path).toString() : QString{};
}

void ThumbnailProvider::requestThumbnail(const QUrl &sourceUrl)
{
    if (!sourceUrl.isValid() || sourceUrl.isEmpty() || !sourceUrl.isLocalFile()) {
        return;
    }
    const QString key = sourceUrl.toString();
    if (m_seen.contains(key) || !cachedThumbnailUrl(sourceUrl).isEmpty()) {
        return;
    }
    m_seen.insert(key);
    m_queue.enqueue(sourceUrl);
    if (m_activeUrl.isEmpty()) {
        processNext();
    }
}

void ThumbnailProvider::processNext()
{
    if (m_queue.isEmpty()) {
        m_activeUrl = QUrl{};
        return;
    }
    m_activeUrl = m_queue.dequeue();
    m_frameCaptured = false;
    m_player->setSource(m_activeUrl);
    m_captureTimeout->start();
}

void ThumbnailProvider::handleMediaStatusChanged(int status)
{
    if (m_activeUrl.isEmpty()) {
        return;
    }
    switch (static_cast<QMediaPlayer::MediaStatus>(status)) {
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferedMedia: {
        const qint64 duration = m_player->duration();
        const qint64 target = duration > 0 ? qMin<qint64>(kThumbnailSeekMs, duration / 4) : 0;
        if (m_player->isSeekable()) {
            m_player->setPosition(target);
        }
        m_player->play();
        break;
    }
    case QMediaPlayer::InvalidMedia:
        failActiveJob();
        break;
    default:
        break;
    }
}

void ThumbnailProvider::handleFrameChanged(const QVideoFrame &frame)
{
    if (m_activeUrl.isEmpty() || m_frameCaptured || !frame.isValid()) {
        return;
    }
    QImage image = frame.toImage();
    if (image.isNull()) {
        return;
    }
    m_frameCaptured = true;
    m_player->pause();

    if (image.width() > kThumbnailMaxDimension || image.height() > kThumbnailMaxDimension) {
        image = image.scaled(kThumbnailMaxDimension, kThumbnailMaxDimension,
                             Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    const QString path = cacheFilePath(m_activeUrl);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QImageWriter writer(path, "jpg");
    writer.setQuality(82);
    finishActiveJob(writer.write(image) ? path : QString{});
}

void ThumbnailProvider::finishActiveJob(const QString &savedPath)
{
    m_captureTimeout->stop();
    const QUrl finishedUrl = m_activeUrl;
    m_activeUrl = QUrl{};
    m_player->stop();
    m_player->setSource(QUrl{});

    if (!savedPath.isEmpty()) {
        emit thumbnailReady(finishedUrl, QUrl::fromLocalFile(savedPath).toString());
    }
    processNext();
}

void ThumbnailProvider::failActiveJob()
{
    if (m_activeUrl.isEmpty()) {
        return;
    }
    finishActiveJob(QString{});
}
