#include "NativeImageFilterHost.h"
#include <QCoreApplication>
#include <QDataStream>
#include <QFile>
#include <cstdio>
#ifdef Q_OS_WIN
#include <io.h>
#include <fcntl.h>
#endif

static QByteArray exact(QFile &input, qsizetype size) {
    QByteArray data;
    while (data.size()<size) {
        auto part=input.read(size-data.size());
        if(part.isEmpty()) return {};
        data+=part;
    }
    return data;
}
int main(int argc, char **argv) {
    QCoreApplication app(argc,argv);
#ifdef Q_OS_WIN
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    if(app.arguments().size()!=4) return 2;
    NativeImageFilterHost host; QString error;
    const auto id=app.arguments()[1];
    if(!host.load(id,app.arguments()[2],&error)) return 3;
    if(!host.configure(id, app.arguments()[3].toUtf8()))return 3;
    host.setEnabled(id,true);
    QFile input,output;
    if(!input.open(stdin,QIODevice::ReadOnly)||!output.open(stdout,QIODevice::WriteOnly)) return 4;
    while(true) {
        const auto header=exact(input,36);
        if(header.isEmpty()) return 0;
        QDataStream in(header); in.setByteOrder(QDataStream::LittleEndian);
        quint32 magic,w,h,stride,format; qint64 timestamp; quint64 size;
        in>>magic>>w>>h>>stride>>format>>timestamp>>size;
        if(magic!=0x414e4652 || !w || !h || w>16384 || h>16384 || format!=1 ||
           stride<w*4 || size!=quint64(stride)*h || size>256*1024*1024) return 5;
        auto bytes=exact(input,qsizetype(size)); if(bytes.size()!=qsizetype(size)) return 6;
        QImage image(reinterpret_cast<const uchar *>(bytes.constData()),int(w),int(h),int(stride),QImage::Format_ARGB32);
        auto result=host.applyOne(id,image,timestamp).convertToFormat(QImage::Format_ARGB32);
        if(result.isNull() || result.size()!=image.size()) return 7;
        QByteArray response; QDataStream out(&response,QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out<<quint32(0x414e5253)<<qint32(0)<<w<<h<<quint32(result.bytesPerLine())<<quint32(1)<<quint64(result.sizeInBytes());
        if(output.write(response)!=response.size() ||
           output.write(reinterpret_cast<const char *>(result.constBits()),result.sizeInBytes())!=result.sizeInBytes() ||
           !output.flush()) return 8;
    }
}
