#include "FrameStore.h"
#include <QUrl>
QMutex FrameStore::mutex;
QHash<QString,QImage> FrameStore::frames;
quint64 FrameStore::revision=0;
QUrl FrameStore::put(const QString &key,QImage image) {
    QMutexLocker lock(&mutex); frames[key]=std::move(image);
    return QUrl("image://frames/"+key+"?"+QString::number(++revision));
}
QImage FrameStore::requestImage(const QString &id,QSize *size,const QSize &requested) {
    QMutexLocker lock(&mutex); QImage image=frames.value(id.section('?',0,0));
    if(size) *size=image.size();
    if(requested.isValid()) image=image.scaled(requested,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    return image;
}
