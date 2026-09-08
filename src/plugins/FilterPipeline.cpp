#include "FilterPipeline.h"
#include "PythonImageFilterHost.h"
#include <QJsonDocument>
#include <QPointer>
#include <QPainter>
#include <memory>

FilterPipeline::FilterPipeline(QObject *parent) : QObject(parent), m_worker(new QObject) {
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.start();
}
FilterPipeline::~FilterPipeline() { m_thread.quit(); m_thread.wait(); }

void FilterPipeline::process(QImage image, qint64 timestamp, QVariantList stack, Completion done) {
    QPointer<FilterPipeline> guard(this);
    QMetaObject::invokeMethod(m_worker, [guard, image=std::move(image), timestamp,
                                      stack=std::move(stack), done=std::move(done)]() mutable {
        // Thread-local ownership guarantees QProcess creation and destruction on this thread.
        thread_local PythonImageFilterHost host;
        thread_local QVariantList previous;
        thread_local QStringList loadErrors;
        if (previous != stack) {
            host.clear(); loadErrors.clear(); previous = stack;
            for (const auto &value : stack) {
                const auto p = value.toMap(); const auto id = p.value("id").toString();
                QString error;
                const bool ok = p.value("native").toBool()
                    ? host.loadNative(id, p.value("path").toString(), &error)
                    : host.load(id, p.value("path").toString(), &error);
                if (ok) {
                    host.setParameters(id, QString::fromUtf8(QJsonDocument::fromVariant(p.value("parameters").toMap()).toJson(QJsonDocument::Compact)));
                    host.setEnabled(id, true);
                } else loadErrors << id + ": " + error;
            }
        }
        QStringList errors = loadErrors;
        QImage current = image.convertToFormat(QImage::Format_ARGB32);
        for (const auto &value : stack) {
            const auto p = value.toMap(); const auto id = p.value("id").toString();
            const double strength = qBound(0.0, p.value("strength", 1.0).toDouble(), 1.0);
            if (strength == 0 || !host.isLoaded(id)) continue;
            QImage next = host.applyOne(id, current, timestamp);
            if (!host.isEnabled(id)) errors << id + ": worker failed or timed out; bypassed. Use Restart in Workspace to retry.";
            if (next.isNull() || next.size() != current.size()) continue;
            if (strength < 1) {
                next = next.convertToFormat(QImage::Format_ARGB32);
                for (int y=0; y<current.height(); ++y) {
                    auto *dst = reinterpret_cast<QRgb *>(next.scanLine(y));
                    const auto *src = reinterpret_cast<const QRgb *>(current.constScanLine(y));
                    for (int x=0; x<current.width(); ++x) {
                        const auto a=src[x], b=dst[x];
                        dst[x]=qRgba(qRound(qRed(a)+(qRed(b)-qRed(a))*strength),
                                     qRound(qGreen(a)+(qGreen(b)-qGreen(a))*strength),
                                     qRound(qBlue(a)+(qBlue(b)-qBlue(a))*strength),qAlpha(a));
                    }
                }
            }
            current = std::move(next);
        }
        if (guard) QMetaObject::invokeMethod(guard, [guard, current=std::move(current), errors, done=std::move(done)]() mutable {
            if (guard) done(std::move(current), errors);
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

void FilterPipeline::releaseWorkers() {
    process({},0,{},[](QImage,QStringList){});
    QMetaObject::invokeMethod(m_worker,[]{},Qt::BlockingQueuedConnection);
}
