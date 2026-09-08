#pragma once
#include <QObject>
#include <functional>
#include <QVariantList>
#include <QImage>
#include <QUrl>
#include <QVector>
class QMediaPlayer; class QVideoSink; class QTimer; class FilterPipeline;
class ExportQueue final: public QObject {
    Q_OBJECT
public:
    explicit ExportQueue(QObject *parent=nullptr);
    void start(QVariantList items,QString directory,QVariantList filters,bool contactSheet);
    void cancel();
    void releaseWorkers();
    bool running() const {return m_running;}
signals:
    void progress(double value,const QString &message);
    void finished(const QString &message);
    void wrote(const QString &path);
private:
    void next();
    void consume(QImage image,qint64 timestamp);
    void finish(QString message);
    void save(QImage image,QString path,std::function<void(bool)> done);
    QMediaPlayer *m_decoder=nullptr;
    QVideoSink *m_sink=nullptr;
    QTimer *m_timeout=nullptr;
    FilterPipeline *m_pipeline=nullptr;
    QVariantList m_items,m_filters;
    QString m_directory;
    QVector<QImage> m_tiles;
    int m_index=0;
    quint64 m_generation=0;
    bool m_running=false,m_waiting=false,m_sheet=false;
};
