#include "SelfTests.h"
#include "player/PlayerController.h"
#include "player/StudioController.h"
#include "player/AudioPlaybackBuffer.h"
#include "player/ExportQueue.h"
#include "plugins/PluginListModel.h"
#include "plugins/FilterPipeline.h"
#include "plugins/PluginValidation.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QJsonDocument>
#include <QTimer>
#include <QAudioBuffer>
#include <QIODevice>
#include <QVideoSink>
#include <QMediaPlayer>
#include <QFileInfo>
#include <cstdio>
#include <stdexcept>
#include <cmath>
#include <cstring>
static void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
static bool waitFor(std::function<bool()> predicate,int timeout=12000) {
    QElapsedTimer timer;timer.start();
    while(timer.elapsed()<timeout){QCoreApplication::processEvents(QEventLoop::AllEvents,20);if(predicate())return true;QThread::msleep(10);}
    return false;
}

class PartialAudioDevice : public QIODevice {
public:
    PartialAudioDevice() { open(QIODevice::WriteOnly); }
    QByteArray received;
    qint64 allowance = 0;
    bool failed = false;
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *data, qint64 size) override {
        if (failed) return -1;
        const auto count = qMin(size, allowance);
        received.append(data, count);
        allowance -= count;
        return count;
    }
};

int runSelfTests(const QString &directory) {
    QDir().mkpath(directory);
    QCoreApplication::setOrganizationName("AndiyaTests");
    QCoreApplication::setApplicationName("Isolated-"+QString::number(QCoreApplication::applicationPid()));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory);
    QFile log(QDir(directory).filePath("results.txt"));log.open(QIODevice::WriteOnly|QIODevice::Truncate);
    auto pass=[&](QString text){log.write(("PASS "+text+"\n").toUtf8());log.flush();};
    try {
        PluginListModel plugins;PlayerController player(&plugins);StudioController studio(&player,&plugins);
        QAudioFormat pcmFormat;pcmFormat.setSampleRate(48000);pcmFormat.setChannelCount(2);
        pcmFormat.setSampleFormat(QAudioFormat::Int16);
        QByteArray pcm(4800*pcmFormat.bytesPerFrame(), char(0));
        for(int i=0;i<4800;++i){
            const qint16 samples[]={qint16(i),qint16(-i)};
            std::memcpy(pcm.data()+i*pcmFormat.bytesPerFrame(),samples,sizeof(samples));
        }
        for(double rate:{0.25,0.5,1.0,1.5,2.0,4.0}) {
            AudioPlaybackBuffer audio;
            // Uneven chunks exercise the stretcher's lookahead and buffer boundaries.
            audio.append(pcm.left(137*4),pcmFormat,rate);
            audio.append(pcm.mid(137*4,1009*4),pcmFormat,rate);
            audio.append(pcm.mid(1146*4),pcmFormat,rate);audio.finish();
            check(audio.queuedBytes()==qint64(4800/rate)*4,"audio duration follows playback rate");
            PartialAudioDevice sink;
            sink.allowance=7;check(audio.drain(&sink),"partial audio write succeeds");
            const auto remaining=audio.queuedBytes();
            check(remaining==qint64(4800/rate)*4-7,"partial writes retain remaining samples");
            check(audio.drain(&sink)&&audio.queuedBytes()==remaining,"zero write retains samples");
            sink.allowance=remaining;check(audio.drain(&sink)&&audio.queuedBytes()==0,"audio retries drain queue");
            if(rate==1.0)check(sink.received==pcm,"normal-speed audio remains byte exact");
            audio.append(pcm,pcmFormat,rate);audio.finish();sink.failed=true;
            check(!audio.drain(&sink)&&audio.queuedBytes()>0,"audio write errors reported without discarding samples");
            audio.clear();check(audio.queuedBytes()==0,"seek clears queued audio");
        }
        pass("audio speeds 0.25–4x, exact duration, partial/zero/error writes and seek reset");

        QAudioFormat toneFormat=pcmFormat;toneFormat.setSampleFormat(QAudioFormat::Float);
        const int toneFrames=48000;QByteArray tones(toneFrames*toneFormat.bytesPerFrame(),Qt::Uninitialized);
        for(int i=0;i<toneFrames;++i){
            const float samples[]={float(0.4*std::sin(2*3.141592653589793*440*i/48000)),
                                   float(0.4*std::sin(2*3.141592653589793*660*i/48000))};
            std::memcpy(tones.data()+i*toneFormat.bytesPerFrame(),samples,sizeof(samples));
        }
        auto frequency=[&](const QByteArray &audio,int channel){
            const int frames=audio.size()/toneFormat.bytesPerFrame();
            double first=-1,last=-1,previous=0;int crossings=0;
            for(int i=frames/4;i<frames*3/4;++i){
                float sample;std::memcpy(&sample,audio.constData()+i*toneFormat.bytesPerFrame()+channel*sizeof(float),sizeof(sample));
                check(std::isfinite(sample)&&std::abs(sample)<=1,"time-stretch samples remain finite and bounded");
                if(i>frames/4&&previous<0&&sample>=0){
                    const double crossing=i-1+(-previous)/(sample-previous);
                    if(first<0)first=crossing;last=crossing;++crossings;
                }
                previous=sample;
            }
            check(crossings>10,"time stretch contains audible tone rather than silence");
            return (crossings-1)*48000.0/(last-first);
        };
        for(double rate:{0.25,0.5,0.75,1.0,1.25,1.5,2.0,3.0,4.0}){
            AudioPlaybackBuffer audio;PartialAudioDevice sink;sink.allowance=10000000;
            for(int offset=0;offset<toneFrames;offset+=997){
                audio.append(tones.mid(offset*toneFormat.bytesPerFrame(),qMin(997,toneFrames-offset)*toneFormat.bytesPerFrame()),toneFormat,rate);
                check(audio.drain(&sink),"streaming time-stretch output drains");
            }
            check(!sink.received.isEmpty(),"time stretch emits audio before the end of the stream");
            audio.finish();check(audio.drain(&sink),"time-stretch tail drains");
            check(sink.received.size()==std::llround(toneFrames/rate)*toneFormat.bytesPerFrame(),"pitch-preserving audio has correct playback duration");
            check(std::abs(frequency(sink.received,0)-440)<6.6&&std::abs(frequency(sink.received,1)-660)<9.9,
                  qPrintable(QString("stereo pitch remains unchanged at %1x").arg(rate)));
            audio.append(tones,toneFormat,rate);audio.clear();audio.append(tones,toneFormat,rate);audio.finish();
            AudioPlaybackBuffer fresh;fresh.append(tones,toneFormat,rate);fresh.finish();
            PartialAudioDevice clearedSink,freshSink;clearedSink.allowance=freshSink.allowance=10000000;
            check(audio.drain(&clearedSink)&&fresh.drain(&freshSink)&&clearedSink.received==freshSink.received,
                  "seek/reset removes previous time-stretch history");
        }
        AudioPlaybackBuffer changedRate;changedRate.append(tones,toneFormat,1.5);changedRate.append(tones,toneFormat,2.0);changedRate.finish();
        check(changedRate.queuedBytes()==(32000+24000)*toneFormat.bytesPerFrame(),"rate changes flush each segment at its own tempo");
        pass("pitch remains 440/660 Hz at 0.25–4x, streaming output, tempo changes and DSP reset");
        const auto appDir=QCoreApplication::applicationDirPath();
        const auto nativeDir=appDir+"/plugins/example.soft-contrast";
        check(PluginValidation::inspect(nativeDir).value("valid").toBool(),"sample native manifest validation");
        pass("sample plugin validates without executing");
        QImage image(128,72,QImage::Format_ARGB32);image.fill(qRgb(30,90,180));
#ifdef ANDIYA_TEST_FILTER_PATH
        if(QFileInfo::exists(QStringLiteral(ANDIYA_TEST_FILTER_PATH))) {
            const QVariantMap failing{{"id","test.native-failure"},{"path",QStringLiteral(ANDIYA_TEST_FILTER_PATH)},
                                      {"native",true},{"strength",1.0}};
            FilterPipeline failurePipeline;
            for(qint64 timestamp:{0,1}) {
                failurePipeline.releaseWorkers();bool complete=false;QImage output;QStringList failure;
                failurePipeline.process(image,timestamp,{failing},[&](QImage result,QStringList errors){output=result;failure=errors;complete=true;});
                check(waitFor([&]{return complete;})&&!failure.isEmpty()&&output==image,"native failure/invalid output is reported and bypassed");
            }
            const auto path=directory+"/failure-input.png";check(image.save(path),"save failing export input");
            ExportQueue failedExport;QString message;int writes=0;
            QObject::connect(&failedExport,&ExportQueue::finished,[&](QString text){message=text;});
            QObject::connect(&failedExport,&ExportQueue::wrote,[&](QString){++writes;});
            failedExport.start({QVariantMap{{"url",QUrl::fromLocalFile(path).toString()},{"image",true}}},
                               directory+"/failed-exports",{failing},false);
            check(waitFor([&]{return !message.isEmpty();})&&message.startsWith("Export stopped:")&&writes==0,"native filter failure stops export without writing unfiltered data");
            pass("native return-code/shape failures reach diagnostics and stop filtered exports");
        }
#endif
        FilterPipeline pipeline;QImage result;QStringList errors;bool ready=false;
        QVariantMap filter{{"id","example.soft-contrast"},{"path",PluginValidation::inspect(nativeDir).value("path").toString()},{"native",true},
                           {"strength",1.0},{"parameters",QVariantMap{{"contrast",1.5},{"warmth",0.0}}}};
        pipeline.process(image,0,{filter},[&](QImage output,QStringList failure){result=output;errors=failure;ready=true;});
        check(waitFor([&]{return ready;}),"native worker response");check(errors.isEmpty(),"native worker error");
        check(result.size()==image.size()&&result.pixel(0,0)!=image.pixel(0,0),"native filter changes pixels at full resolution");
        pass("native process worker, parameters and full-resolution processing");
        filter["strength"]=0.0;ready=false;pipeline.process(image,0,{filter},[&](QImage output,QStringList){result=output;ready=true;});
        check(waitFor([&]{return ready;})&&result==image,"zero-strength bypass");pass("strength control");

        QVariantMap pythonFilter{{"id","example.python-grayscale"},{"path",appDir+"/plugins/example.python-grayscale/filter.py"},{"native",false},{"strength",1.0}};
        ready=false;pipeline.process(image,0,{pythonFilter},[&](QImage output,QStringList failure){result=output;errors=failure;ready=true;});
        check(waitFor([&]{return ready;})&&errors.isEmpty(),"Python worker success");
        check(qRed(result.pixel(0,0))==qBlue(result.pixel(0,0))&&result!=image,"Python grayscale pixels");
        QFile bypass(directory+"/bypass.py");bypass.open(QIODevice::WriteOnly);bypass.write("def process_frame(*args): return None\n");bypass.close();
        pythonFilter["id"]="test.bypass";pythonFilter["path"]=bypass.fileName();ready=false;
        pipeline.process(image,0,{pythonFilter},[&](QImage output,QStringList failure){result=output;errors=failure;ready=true;});
        check(waitFor([&]{return ready;})&&errors.isEmpty()&&result==image,"intentional Python bypass stays healthy");
        pass("Python worker processing, default parameters and intentional bypass");
        QTimer heartbeat;heartbeat.setInterval(10);int beats=0;QObject::connect(&heartbeat,&QTimer::timeout,[&]{++beats;});heartbeat.start();
        QFile crashing(directory+"/crash.py");check(crashing.open(QIODevice::WriteOnly),"create crash fixture");
        crashing.write("import os\ndef process_frame(*args):\n    os._exit(37)\n");crashing.close();
        QVariantMap crashFilter{{"id","test.crash"},{"path",crashing.fileName()},{"native",false},{"strength",1.0}};
        ready=false;pipeline.process(image,0,{crashFilter},[&](QImage output,QStringList failure){result=output;errors=failure;ready=true;});
        check(waitFor([&]{return ready;})&&!errors.isEmpty()&&result==image,"crash is bypassed");check(beats>0,"UI heartbeat during worker");pass("plugin crash containment and responsive event loop");

        const auto video=QUrl::fromLocalFile(directory+"/fixture.avi");
        player.openMedia(video);
        check(waitFor([&]{return player.position()>500&&player.videoWidth()==128;}),"fixture playback");
        player.togglePlayback();waitFor([&]{return !player.playing();});
        player.stepFrame(1);
        check(waitFor([&]{return !player.playing();}),"forward frame pause");
        const auto first=player.position();player.stepFrame(1);
        check(waitFor([&]{return !player.playing();}),"second forward frame");
        const auto second=player.position();
        check(second-first==100,"frame stepping follows 10 fps timestamps");
        player.stepFrame(-1);check(waitFor([&]{return !player.playing();}),"backward frame pause");
        check(player.position()==first,"backward frame is previous decoded timestamp");pass("forward and backward frame navigation at 10 fps");

        player.addBookmark("Test note");check(player.bookmarks().size()==1,"bookmark stored");
        player.editBookmark(0,"Edited note");check(player.bookmarks()[0].toMap().value("label").toString()=="Edited note","bookmark editing");
        player.setLoopStart();player.seek(first+1000);waitFor([&]{return player.position()>=first+1000;});player.setLoopEnd();
        check(player.property("loopEnabled").toBool(),"A-B loop enabled");player.clearLoop();pass("bookmarks, notes and loop markers");
        const auto exportDir=directory+"/exports";player.setCaptureDirectory(QUrl::fromLocalFile(exportDir));
        player.seek(0);waitFor([&]{return player.position()==0;});
        player.startBatchCapture(1,3,false,true);
        check(waitFor([&]{return !player.property("batchRunning").toBool();},25000),"batch completes");
        check(player.property("batchStatus").toString().startsWith("Export complete"),"batch success status");
        auto pngs=QDir(exportDir).entryList({"*.png"},QDir::Files);check(pngs.size()==4,"three captures plus contact sheet");
        for(const auto &name:pngs)if(!name.startsWith("contact-sheet"))check(QImage(exportDir+"/"+name).size()==QSize(128,72),"export resolution");
        pass("interval export count, full resolution and contact sheet");
        {
            ExportQueue staleExport;QString message;int writes=0,staleFrames=0;
            QObject::connect(&staleExport,&ExportQueue::finished,[&](QString text){message=text;});
            QObject::connect(&staleExport,&ExportQueue::wrote,[&](QString){++writes;});
            QObject::connect(&staleExport,&ExportQueue::progress,[&](double value,QString text){
                if(value<=0||!text.startsWith("Exporting"))return;
                // Retired decoders can emit late status/frame signals during item transitions.
                for(auto *decoder:staleExport.findChildren<QMediaPlayer *>())decoder->mediaStatusChanged(QMediaPlayer::LoadedMedia);
                QVideoFrame lateFrame(image);lateFrame.setStartTime(0);lateFrame.setEndTime(100000);
                for(auto *sink:staleExport.findChildren<QVideoSink *>()){
                    ++staleFrames;sink->videoFrameChanged(lateFrame);
                }
            });
            staleExport.start({QVariantMap{{"url",video.toString()},{"position",0}},
                               QVariantMap{{"url",video.toString()},{"position",1000}}},
                              directory+"/stale-exports",{},false);
            check(waitFor([&]{return !message.isEmpty();},25000)&&message.startsWith("Export complete")&&writes==2&&staleFrames>0,
                  "retired decoder callbacks do not restart exports or affect next item");
            pass("export item transitions ignore retired decoder status/frame callbacks");
        }
        player.startBatchCapture(1,6,false,false);player.cancelBatch();
        check(!player.property("batchRunning").toBool(),"cancel batch");pass("export cancellation");

        const auto imagePath=directory+"/image.png";image.save(imagePath);
        player.exportImages({QUrl::fromLocalFile(imagePath)});
        check(waitFor([&]{return !player.property("batchRunning").toBool();}),"image export");
        check(QDir(exportDir).entryList({"*.png"},QDir::Files).size()>=5,"image output exists");pass("image batch export");

#ifdef Q_OS_WIN
        studio.saveProfile("Test stream","https://user:secret@example.test/video?token=private");
        check(studio.profiles().size()==1,"profile saved");
        check(!QJsonDocument::fromVariant(QSettings().value("streams/profiles")).toJson().contains("secret@"),"profile secrets encrypted");
#else
        check(!studio.secureProfilesAvailable(),"secure profiles unavailable on this platform");
#endif
        player.addToPlaylist(QUrl("https://user:secret@example.test/video?token=private"));
        const auto workspace=QUrl::fromLocalFile(directory+"/workspace.andiya");
        studio.exportWorkspace(workspace);QFile saved(workspace.toLocalFile());check(saved.open(QIODevice::ReadOnly),"workspace exported");
        const auto bytes=saved.readAll();saved.close();
        check(!bytes.contains("secret")&&!bytes.contains("private"),"workspace excludes credentials");
        const int bookmarkCount=player.bookmarks().size();player.removeBookmark(0);player.clearPlaylist();studio.importWorkspace(workspace);
        check(player.bookmarks().size()==bookmarkCount&&!player.playlist().isEmpty(),"workspace round trip");
        pass("platform profile capability, history redaction and workspace round trip");
        check(!studio.setShortcut("play","Left").isEmpty(),"duplicate shortcut rejected");
        check(studio.setShortcut("play","Ctrl+P").isEmpty(),"shortcut persisted");pass("configurable keyboard shortcuts");

        // Presets restore order, parameters and enablement only for approved content.
        int nativeIndex=-1;
        for(const auto &value:plugins.details())if(value.toMap().value("id").toString()=="example.soft-contrast")nativeIndex=value.toMap().value("index").toInt();
        check(nativeIndex>=0,"native sample discovered");
        auto detail=plugins.details()[nativeIndex].toMap();
        plugins.approvePlugin(detail.value("id").toString(),detail.value("fingerprint").toString());
        plugins.setParameter(nativeIndex,"contrast",1.7);plugins.setStrength(nativeIndex,0.4);
        plugins.savePreset("Test preset");plugins.setStrength(nativeIndex,0.9);plugins.loadPreset("Test preset");
        bool presetOkay=false;for(const auto &value:plugins.details()){
            const auto p=value.toMap();if(p.value("id").toString()=="example.soft-contrast")
                presetOkay=p.value("enabled").toBool()&&qAbs(p.value("strength").toDouble()-0.4)<0.001&&qAbs(p.value("values").toMap().value("contrast").toDouble()-1.7)<0.001;
        }
        check(presetOkay,"preset restores approved stack and parameters");pass("plugin approval and preset persistence");
        for(int i=0;i<plugins.rowCount();++i)plugins.setPluginEnabled(i,false);
        // A valid local package can update and roll back without executing during discovery.
        const auto folder=directory+"/install-fixture";QDir().mkpath(folder);
        QFile script(folder+"/filter.py");script.open(QIODevice::WriteOnly);script.write("def process_frame(*args): return args[-1]\n");script.close();
        QVariantMap manifest{{"id","test.install"},{"name","Install test"},{"version","1.0.0"},{"andiyaApi","1.0"},
                             {"execution","python"},{"runtime","process"},{"entrypoint","filter.py"},{"capabilities",QStringList{"image-filter"}}};
        auto writeManifest=[&]{QFile f(folder+"/plugin.json");f.open(QIODevice::WriteOnly);f.write(QJsonDocument::fromVariant(manifest).toJson());};
        writeManifest();check(plugins.installPlugin(QUrl::fromLocalFile(folder)).isEmpty(),"local install");
        manifest["version"]="1.1.0";writeManifest();check(plugins.installPlugin(QUrl::fromLocalFile(folder)).isEmpty(),"local update");
        check(plugins.rollbackPlugin("test.install").isEmpty(),"rollback");
        bool restored=false;for(const auto &value:plugins.details())if(value.toMap().value("id").toString()=="test.install")
            restored=value.toMap().value("version").toString()=="1.0.0"&&!value.toMap().value("enabled").toBool();
        check(restored,"rollback version and disabled discovery");pass("local install, update and rollback");

        // A manually opened source must not inherit automatic playlist advancement.
        const auto secondPath=directory+"/second.avi",manualPath=directory+"/manual.avi";
        check(QFile::copy(video.toLocalFile(),secondPath)&&QFile::copy(video.toLocalFile(),manualPath),"create playlist fixtures");
        player.clearPlaylist();player.addToPlaylist(video);player.addToPlaylist(QUrl::fromLocalFile(secondPath));
        player.playPlaylistIndex(0);check(player.playlistIndex()==0,"playlist selection retained");
        check(waitFor([&]{return player.duration()>0&&player.seekable();}),"playlist media loads");
        player.seek(5900);if(!player.playing())player.togglePlayback();
        check(waitFor([&]{return player.source()==QUrl::fromLocalFile(secondPath);}),"playlist still advances automatically");
        player.openMedia(QUrl::fromLocalFile(manualPath));check(player.playlistIndex()==-1,"manual open clears playlist selection");
        check(waitFor([&]{return player.duration()>0&&player.seekable();}),"manually opened media loads");
        player.seek(5900);if(!player.playing())player.togglePlayback();
        check(waitFor([&]{return !player.playing();})&&player.source()==QUrl::fromLocalFile(manualPath),"manual playback does not advance old playlist");
        pass("playlist advancement and manual-open selection reset");

        const auto slowFolder=directory+"/slow-preview";QDir().mkpath(slowFolder);
        QFile slowScript(slowFolder+"/filter.py");check(slowScript.open(QIODevice::WriteOnly),"create slow preview filter");
        slowScript.write("import time\ndef process_frame(*args):\n    time.sleep(0.15)\n    return args[-1]\n");slowScript.close();
        manifest["id"]="test.slow-preview";manifest["version"]="1.0.0";
        QFile slowManifest(slowFolder+"/plugin.json");check(slowManifest.open(QIODevice::WriteOnly),"create slow filter manifest");
        slowManifest.write(QJsonDocument::fromVariant(manifest).toJson());slowManifest.close();
        check(plugins.installPlugin(QUrl::fromLocalFile(slowFolder)).isEmpty(),"install slow preview filter");
        for(const auto &value:plugins.details()){
            const auto detail=value.toMap();if(detail.value("id").toString()=="test.slow-preview")
                plugins.approvePlugin(detail.value("id").toString(),detail.value("fingerprint").toString());
        }
        {
            PlayerController previewPlayer(&plugins);QVideoSink displaySink;QObject displayOutput;
            displayOutput.setProperty("videoSink",QVariant::fromValue(&displaySink));previewPlayer.attachVideoOutput(&displayOutput);
            // Inject two decoded frames while paused; the second arrives during slow processing.
            auto *decoderSink=previewPlayer.findChild<QVideoSink *>();check(decoderSink,"find decoder sink");
            QVideoFrame oldFrame(image);oldFrame.setStartTime(100000);
            QImage latest=image;latest.fill(qRgb(200,10,50));
            QVideoFrame latestFrame(latest);latestFrame.setStartTime(200000);
            decoderSink->setVideoFrame(oldFrame);decoderSink->setVideoFrame(latestFrame);
            check(waitFor([&]{return displaySink.videoFrame().startTime()==200000;}),
                  qPrintable(QString("paused preview reaches newest timestamp (actual %1, filters %2)")
                             .arg(displaySink.videoFrame().startTime()).arg(previewPlayer.diagnostics().value("Filters").toString())));
            check(displaySink.videoFrame().toImage().convertToFormat(latest.format())==latest,
                  "paused preview contains newest pixels after slow filter");
            check(previewPlayer.diagnostics().value("Filters").toString()=="Healthy","slow preview filter ran successfully");
        }
        pass("slow filtered preview catches up to the latest paused frame");
        for(int i=0;i<plugins.rowCount();++i)plugins.setPluginEnabled(i,false);
        QFile hanging(directory+"/hang.py");hanging.open(QIODevice::WriteOnly);hanging.write("import time\ndef process_frame(*args):\n    time.sleep(30)\n");hanging.close();
        QVariantMap hangFilter{{"id","test.hang"},{"path",hanging.fileName()},{"native",false},{"strength",1.0}};
        ready=false;const int previousBeats=beats;
        pipeline.process(image,0,{hangFilter},[&](QImage output,QStringList failure){result=output;errors=failure;ready=true;});
        check(waitFor([&]{return ready;},15000)&&!errors.isEmpty(),"worker timeout");
        check(beats-previousBeats>50,qPrintable("event loop stays responsive during timeout: "+errors.join("; ")));pass("hung worker timeout and GUI responsiveness");

        studio.deleteProfile(0);
        log.write("ALL TESTS PASSED\n");log.flush();return 0;
    } catch(const std::exception &error) {
        log.write(QByteArray("FAIL ")+error.what()+"\n");log.flush();return 1;
    }
}
