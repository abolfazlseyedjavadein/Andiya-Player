#include "SelfTests.h"
#include "player/PlayerController.h"
#include "player/StudioController.h"
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
#include <cstdio>
#include <stdexcept>
static void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
static bool waitFor(std::function<bool()> predicate,int timeout=12000) {
    QElapsedTimer timer;timer.start();
    while(timer.elapsed()<timeout){QCoreApplication::processEvents(QEventLoop::AllEvents,20);if(predicate())return true;QThread::msleep(10);}
    return false;
}
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
        const auto appDir=QCoreApplication::applicationDirPath();
        const auto nativeDir=appDir+"/plugins/example.soft-contrast";
        check(PluginValidation::inspect(nativeDir).value("valid").toBool(),"sample native manifest validation");
        pass("sample plugin validates without executing");
        QImage image(128,72,QImage::Format_ARGB32);image.fill(qRgb(30,90,180));
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
