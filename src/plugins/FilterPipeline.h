#pragma once
#include <QObject>
#include <QThread>
#include <QImage>
#include <QVariantList>
#include <functional>

// All child-process I/O and pixel work belongs to this serial worker.
class FilterPipeline final : public QObject {
    Q_OBJECT
public:
    explicit FilterPipeline(QObject *parent = nullptr);
    ~FilterPipeline() override;
    using Completion = std::function<void(QImage, QStringList)>;
    void releaseWorkers();
    void process(QImage image, qint64 timestampUs, QVariantList stack, Completion done);
private:
    QThread m_thread;
    QObject *m_worker = nullptr;
};
