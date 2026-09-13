#include <QGuiApplication>
#include <QFileInfo>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <QIcon>
#include "SelfTests.h"
#include <QQuickWindow>
#include <QDir>
#include <QSettings>
#include <QFontDatabase>
#include "player/FrameStore.h"
#include "player/StudioController.h"

#include "player/PlayerController.h"
#include "player/ScreenshotListModel.h"
#include "player/ThumbnailProvider.h"
#include "plugins/PluginListModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setApplicationName(QStringLiteral("Andiya"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Andiya Media Player"));
    QGuiApplication::setOrganizationName(QStringLiteral("Andiya"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("andiya.io"));

    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    if(app.arguments().size()==3 && app.arguments()[1]=="--self-test") return runSelfTests(app.arguments()[2]);

    const bool uiSmoke=app.arguments().size()==3&&app.arguments()[1]=="--ui-smoke";
    if(uiSmoke) {
#ifdef Q_OS_WIN
        QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");
        QFontDatabase::addApplicationFont("C:/Windows/Fonts/seguisym.ttf");
#endif
        QCoreApplication::setOrganizationName("AndiyaUiTests");
        QCoreApplication::setApplicationName("Smoke-"+QString::number(QCoreApplication::applicationPid()));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,app.arguments()[2]);
    }
    PluginListModel plugins;
    PlayerController player(&plugins);
    StudioController studio(&player,&plugins);
    app.setWindowIcon(QIcon(":/assets/andiya-icon.png"));
    ThumbnailProvider thumbnails;
    ScreenshotListModel screenshots;
    // PNG metadata scans can be expensive for a large capture library.
    // Start after the main window is created so it appears promptly.
    QTimer::singleShot(250, &screenshots, [&player, &screenshots] {
        screenshots.setDirectory(player.captureDirectoryPath());
    });
    QObject::connect(&player, &PlayerController::captureDirectoryChanged, &screenshots, [&player, &screenshots] {
        screenshots.setDirectory(player.captureDirectoryPath());
    });
    QObject::connect(&player, &PlayerController::lastCapturePathChanged, &screenshots,
                     &ScreenshotListModel::refresh);

    QQmlApplicationEngine engine;
    engine.addImageProvider("frames",new FrameStore);
    engine.rootContext()->setContextProperty("studio",&studio);
    engine.rootContext()->setContextProperty(QStringLiteral("player"), &player);
    engine.rootContext()->setContextProperty(QStringLiteral("pluginModel"), &plugins);
    engine.rootContext()->setContextProperty(QStringLiteral("thumbnails"), &thumbnails);
    engine.rootContext()->setContextProperty(QStringLiteral("screenshots"), &screenshots);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("Andiya"), QStringLiteral("Main"));

    if(uiSmoke && !engine.rootObjects().isEmpty()) {
        const QString directory=app.arguments()[2];QDir().mkpath(directory);
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QTimer::singleShot(300,window,[window,directory,&studio]{
            auto *dialog=window->findChild<QObject *>("workspaceDialog");
            auto *tabs=window->findChild<QObject *>("workspaceTabs");
            if(!dialog||!tabs){QCoreApplication::exit(3);return;}
            QMetaObject::invokeMethod(dialog,"open");
            for(int i=0;i<6;++i) {
                QTimer::singleShot(500+i*350,window,[tabs,i]{tabs->setProperty("currentIndex",i);});
                QTimer::singleShot(750+i*350,window,[window,directory,i]{window->grabWindow().save(directory+QString("/workspace-%1.png").arg(i));});
            }
            QTimer::singleShot(2850,window,[window,tabs,&studio]{tabs->setProperty("currentIndex",5);window->setWidth(980);studio.setTextScale(1.5);});
            QTimer::singleShot(3300,window,[window,directory]{window->grabWindow().save(directory+"/workspace-large-text.png");QCoreApplication::quit();});
        });
    }
    if (!uiSmoke && app.arguments().size() > 1) {
        const QString mediaArgument = app.arguments().at(1);
        const QString subtitleArgument = app.arguments().size() > 2
                                             ? app.arguments().at(2)
                                             : QString{};
        QTimer::singleShot(0, &player, [&player, mediaArgument, subtitleArgument] {
            const QFileInfo file(mediaArgument);
            player.openMedia(file.exists() ? QUrl::fromLocalFile(file.absoluteFilePath())
                                           : QUrl::fromUserInput(mediaArgument));
            if (!subtitleArgument.isEmpty()) {
                const QFileInfo subtitleFile(subtitleArgument);
                player.openSubtitle(subtitleFile.exists()
                                        ? QUrl::fromLocalFile(subtitleFile.absoluteFilePath())
                                        : QUrl::fromUserInput(subtitleArgument));
            }
        });
    }
    return app.exec();
}
