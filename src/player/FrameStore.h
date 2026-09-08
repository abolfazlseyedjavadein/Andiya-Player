#pragma once
#include <QQuickImageProvider>
#include <QMutex>
#include <QHash>
class FrameStore final : public QQuickImageProvider {
public:
    FrameStore() : QQuickImageProvider(QQuickImageProvider::Image) {}
    static QUrl put(const QString &key, QImage image);
    QImage requestImage(const QString &id, QSize *size, const QSize &requested) override;
private:
    static QMutex mutex;
    static QHash<QString,QImage> frames;
    static quint64 revision;
};
