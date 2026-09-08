#include "ExportQueue.h"
#include "plugins/FilterPipeline.h"
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <QSaveFile>
#include <QThreadPool>
#include <QPointer>
#include <QUuid>
#include <cmath>
#include <QRegularExpression>

ExportQueue::ExportQueue(QObject *parent):QObject(parent),m_timeout(new QTimer(this)),m_pipeline(new FilterPipeline(this)) {
    m_timeout->setSingleShot(true);m_timeout->setInterval(20000);
    connect(m_timeout,&QTimer::timeout,this,[this]{finish("Export timed out while decoding a frame.");});
}
void ExportQueue::start(QVariantList items,QString directory,QVariantList filters,bool sheet) {
    if(m_running || items.isEmpty() || items.size()>200) return;
    ++m_generation;m_items=std::move(items);m_directory=std::move(directory);m_filters=std::move(filters);
    m_sheet=sheet;m_index=0;m_tiles.clear();m_running=true;
    if(!QDir().mkpath(m_directory)){finish("Cannot create the export directory.");return;}
    emit progress(0,"Preparing export");next();
}
void ExportQueue::cancel(){if(m_running)finish("Export cancelled. Completed files were kept.");}
void ExportQueue::finish(QString message) {
    ++m_generation;m_running=false;m_waiting=false;m_timeout->stop();
    if(m_decoder){m_decoder->stop();m_decoder->deleteLater();m_decoder=nullptr;}
    if(m_sink){m_sink->deleteLater();m_sink=nullptr;}
    m_tiles.clear();emit finished(message);
}
void ExportQueue::save(QImage image,QString path,std::function<void(bool)> done) {
    QPointer<ExportQueue> guard(this);const auto generation=m_generation;
    QThreadPool::globalInstance()->start([guard,generation,image=std::move(image),path,done=std::move(done)]{
        QSaveFile file(path);
        const bool ok=file.open(QIODevice::WriteOnly)&&image.save(&file,"PNG")&&file.commit();
        if(guard)QMetaObject::invokeMethod(guard,[guard,generation,path,ok,done]{
            if(!guard||generation!=guard->m_generation)return;
            if(ok)emit guard->wrote(path);done(ok);
        });
    });
}
void ExportQueue::next() {
    if(!m_running)return;
    if(m_decoder){m_decoder->stop();m_decoder->deleteLater();m_decoder=nullptr;}
    if(m_sink){m_sink->deleteLater();m_sink=nullptr;}
    if(m_index==m_items.size()) {
        if(m_sheet&&!m_tiles.isEmpty()) {
            const int columns=qMin(4,int(m_tiles.size())), rows=(m_tiles.size()+columns-1)/columns;
            QImage sheet(columns*320,rows*204,QImage::Format_RGB32);sheet.fill(QColor("#161C26"));
            QPainter painter(&sheet);for(int i=0;i<m_tiles.size();++i)painter.drawImage((i%columns)*320,(i/columns)*204,m_tiles[i]);painter.end();
            save(sheet,QDir(m_directory).filePath("contact-sheet-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".png"),
                 [this](bool ok){finish(ok?"Export complete, including contact sheet.":"Could not write the contact sheet.");});
        } else finish("Export complete.");
        return;
    }
    emit progress(double(m_index)/m_items.size(),QString("Exporting %1 of %2").arg(m_index+1).arg(m_items.size()));
    const auto item=m_items[m_index].toMap();const QUrl url(item.value("url").toString());
    if(item.value("image").toBool()) {
        QImageReader reader(url.toLocalFile());reader.setAutoTransform(true);
        const auto image=reader.read();
        if(image.isNull()){finish("Cannot read image: "+url.fileName());return;}
        consume(image,0);return;
    }
    m_decoder=new QMediaPlayer(this);m_sink=new QVideoSink(this);m_decoder->setVideoSink(m_sink);
    m_waiting=false;
    const auto generation=m_generation;
    connect(m_decoder,&QMediaPlayer::errorOccurred,this,[this,generation](QMediaPlayer::Error,const QString &){
        if(generation==m_generation)finish("Cannot decode this export item. Check its file or stream availability.");
    });
    connect(m_decoder,&QMediaPlayer::mediaStatusChanged,this,[this,generation,item](QMediaPlayer::MediaStatus status){
        if(generation!=m_generation||!m_decoder)return;
        if(status==QMediaPlayer::LoadedMedia) {
            const auto target=item.value("position").toLongLong();
            if(target>0&&!m_decoder->isSeekable()){finish("Export requires a seekable source.");return;}
            m_decoder->setPosition(target);m_waiting=true;m_decoder->play();
        } else if(status==QMediaPlayer::EndOfMedia&&m_waiting)finish("The requested export position is beyond the last frame.");
    });
    connect(m_sink,&QVideoSink::videoFrameChanged,this,[this,generation,item](const QVideoFrame &frame){
        if(generation!=m_generation||!m_waiting||!frame.isValid())return;
        const auto target=item.value("position").toLongLong()*1000;
        if(frame.startTime()<0 || (frame.startTime()<target && frame.endTime()<=target))return;
        m_waiting=false;m_timeout->stop();m_decoder->pause();
        consume(frame.toImage(),qMax<qint64>(0,frame.startTime()/1000));
    });
    m_timeout->start();m_decoder->setSource(url);
}
void ExportQueue::consume(QImage image,qint64 timestamp) {
    if(image.isNull()){finish("The decoder returned an empty frame.");return;}
    const auto generation=m_generation;const auto item=m_items[m_index].toMap();
    m_pipeline->process(image,timestamp*1000,m_filters,[this,generation,item,timestamp](QImage result,QStringList errors){
        if(generation!=m_generation||!m_running)return;
        if(!errors.isEmpty()){finish("Export stopped: "+errors.join("; "));return;}
        if(m_sheet){
            QImage tile(320,204,QImage::Format_RGB32);tile.fill(QColor("#161C26"));QPainter p(&tile);
            auto thumb=result.scaled(312,174,Qt::KeepAspectRatio,Qt::SmoothTransformation);
            p.drawImage((320-thumb.width())/2,(176-thumb.height())/2,thumb);
            p.setPen(Qt::white);p.drawText(QRect(5,178,310,24),Qt::AlignCenter,QString("%1  •  %2 ms").arg(m_index+1).arg(timestamp));p.end();
            m_tiles<<tile;
        }
        QString base=QFileInfo(QUrl(item.value("url").toString()).path()).completeBaseName();
        base.replace(QRegularExpression("[^A-Za-z0-9_-]"),"_");
        const auto path=QDir(m_directory).filePath(base.left(80)+"_"+QString::number(timestamp)+"ms_"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".png");
        save(result,path,[this](bool ok){if(!ok){finish("Cannot write the export image.");return;}++m_index;next();});
    });
}

void ExportQueue::releaseWorkers(){cancel();m_pipeline->releaseWorkers();}
