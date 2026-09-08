#include "StudioController.h"
#include "PlayerController.h"
#include "plugins/PluginListModel.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QTemporaryDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QKeySequence>
#include <QRegularExpression>
#include <QTimer>
#include <cmath>
#include <QSet>
#include <memory>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <wincrypt.h>
#endif

static QVariantMap defaultKeys() {
    return {{"play","Space"},{"back","Left"},{"forward","Right"},{"previousFrame",","},
            {"nextFrame","."},{"capture","S"},{"compare","K"},{"fullscreen","F"},
            {"workspace","Ctrl+Shift+W"}};
}
StudioController::StudioController(PlayerController *player,PluginListModel *plugins,QObject *parent)
    :QObject(parent),m_player(player),m_plugins(plugins),m_network(new QNetworkAccessManager(this)) {}
QUrl StudioController::publicUrl(QUrl url) {
    if(!url.isLocalFile()){url.setUserInfo({});url.setQuery({});url.setFragment({});}
    return url;
}
void StudioController::report(QString text){m_status=std::move(text);emit statusChanged();}
QVariantList StudioController::profiles()const {
    auto entries=QSettings().value("streams/profiles").toList();
    for(auto &value:entries){auto map=value.toMap();map.remove("secret");value=map;}
    return entries;
}
double StudioController::textScale()const {return qBound(0.85,QSettings().value("appearance/textScale",1.0).toDouble(),1.5);}
void StudioController::setTextScale(double value) {
    if(!std::isfinite(value))return;
    QSettings().setValue("appearance/textScale",qBound(0.85,value,1.5));emit preferencesChanged();
}
QVariantMap StudioController::shortcuts()const {
    auto keys=defaultKeys();const auto saved=QSettings().value("accessibility/shortcuts").toMap();
    for(auto it=saved.begin();it!=saved.end();++it)if(keys.contains(it.key()))keys[it.key()]=it.value();
    return keys;
}
QString StudioController::setShortcut(const QString &action,const QString &sequence) {
    auto keys=shortcuts();if(!keys.contains(action))return "Unknown action.";
    const QKeySequence key(sequence,QKeySequence::PortableText);
    const QString normalized=key.toString(QKeySequence::PortableText);
    if(key.isEmpty()||key.count()!=1||normalized.isEmpty())return "Enter one valid key combination.";
    const QStringList reserved={"Ctrl+O","Ctrl+U","Ctrl+,","F1","F11","Escape","Ctrl+Shift+S","Ctrl+Alt+S","Ctrl+Up","Ctrl+Down","C","?","Shift+/"};
    if(reserved.contains(normalized,Qt::CaseInsensitive))return "That key is reserved by another app command.";
    for(auto it=keys.begin();it!=keys.end();++it)if(it.key()!=action&&it.value().toString()==normalized)return "That key is already assigned to "+it.key()+".";
    keys[action]=normalized;QSettings().setValue("accessibility/shortcuts",keys);emit preferencesChanged();return {};
}
void StudioController::resetShortcuts(){QSettings().remove("accessibility/shortcuts");emit preferencesChanged();}
void StudioController::saveProfile(const QString &name,const QString &text) {
    const QUrl url(text.trimmed(),QUrl::StrictMode);
    if(name.trimmed().isEmpty()||name.size()>100||!url.isValid()||url.scheme().isEmpty()||url.isLocalFile()||text.size()>8000){
        report("Enter a profile name and a valid network URL.");return;
    }
    QByteArray encrypted;
#ifdef Q_OS_WIN
    QByteArray plain=url.toEncoded();DATA_BLOB input{DWORD(plain.size()),reinterpret_cast<BYTE *>(plain.data())},output{};
    if(!CryptProtectData(&input,L"Andiya stream",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output)){report("Windows could not protect the stream credentials.");return;}
    encrypted=QByteArray(reinterpret_cast<char *>(output.pbData),output.cbData);LocalFree(output.pbData);
    SecureZeroMemory(plain.data(),plain.size());
#else
    report("Secure stream storage currently requires Windows.");return;
#endif
    auto entries=QSettings().value("streams/profiles").toList();
    QVariantMap entry{{"name",name.trimmed()},{"address",publicUrl(url).toString()},{"secret",encrypted.toBase64()}};
    int index=-1;for(int i=0;i<entries.size();++i)if(entries[i].toMap().value("name").toString()==name.trimmed())index=i;
    if(index>=0)entries[index]=entry;else {if(entries.size()>=100){report("A maximum of 100 profiles is supported.");return;}entries<<entry;}
    QSettings().setValue("streams/profiles",entries);emit profilesChanged();report("Profile saved. The complete address is encrypted for this Windows account.");
}
void StudioController::openProfile(int index) {
    const auto entries=QSettings().value("streams/profiles").toList();if(index<0||index>=entries.size())return;
#ifdef Q_OS_WIN
    auto encrypted=QByteArray::fromBase64(entries[index].toMap().value("secret").toByteArray());
    DATA_BLOB input{DWORD(encrypted.size()),reinterpret_cast<BYTE *>(encrypted.data())},output{};
    if(!CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output)){report("Cannot decrypt this profile with the current Windows account.");return;}
    QByteArray plain(reinterpret_cast<char *>(output.pbData),output.cbData);SecureZeroMemory(output.pbData,output.cbData);LocalFree(output.pbData);
    m_player->openNetworkStream(QString::fromUtf8(plain));SecureZeroMemory(plain.data(),plain.size());report("Opening saved stream.");
#endif
}
void StudioController::deleteProfile(int index) {
    auto entries=QSettings().value("streams/profiles").toList();if(index<0||index>=entries.size())return;
    entries.removeAt(index);QSettings().setValue("streams/profiles",entries);emit profilesChanged();report("Profile deleted.");
}
void StudioController::exportWorkspace(const QUrl &url) {
    if(!url.isLocalFile())return;
    QVariantMap data=m_player->workspaceData();data["format"]="andiya-workspace";data["version"]=1;
    data["textScale"]=textScale();data["shortcuts"]=shortcuts();
    for(const QString key:{"bookmarks","playlist"}) {
        auto items=data.value(key).toList();
        for(auto &value:items){auto item=value.toMap();item["url"]=publicUrl(QUrl(item.value("url").toString())).toString();value=item;}
        data[key]=items;
    }
    QSaveFile file(url.toLocalFile());const auto bytes=QJsonDocument::fromVariant(data).toJson();
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){report("Could not save workspace.");return;}
    report("Workspace exported. Stream credentials and executable plugins are excluded.");
}
void StudioController::importWorkspace(const QUrl &url) {
    if(!url.isLocalFile())return;
    QFile file(url.toLocalFile());if(!file.open(QIODevice::ReadOnly)||file.size()>4*1024*1024){report("Cannot read this workspace or it exceeds 4 MB.");return;}
    const auto data=QJsonDocument::fromJson(file.readAll()).toVariant().toMap();
    if(data.value("format").toString()!="andiya-workspace"||data.value("version").toInt()!=1){report("Unsupported workspace format.");return;}
    // Validate all shortcut values before making changes.
    auto keys=defaultKeys();const auto importedKeys=data.value("shortcuts").toMap();
    for(auto it=importedKeys.begin();it!=importedKeys.end();++it)keys[it.key()]=it.value();
    QSet<QString> used;
    const QStringList reserved={"Ctrl+O","Ctrl+U","Ctrl+,","F1","F11","Escape","Ctrl+Shift+S","Ctrl+Alt+S","Ctrl+Up","Ctrl+Down","C","?","Shift+/"};
    for(auto it=keys.begin();it!=keys.end();++it) {
        const QKeySequence key(it.value().toString(),QKeySequence::PortableText);
        const auto normalized=key.toString(QKeySequence::PortableText);
        if(!defaultKeys().contains(it.key())||key.isEmpty()||key.count()!=1||used.contains(normalized)||reserved.contains(normalized,Qt::CaseInsensitive)){report("Invalid or duplicate workspace shortcuts.");return;}
        used.insert(normalized);
    }
    QString error;if(!m_player->restoreWorkspace(data,&error)){report(error);return;}
    QSettings().setValue("accessibility/shortcuts",keys);setTextScale(data.value("textScale",1.0).toDouble());
    report("Workspace imported. Existing bookmarks, playlist, presets and preferences were replaced.");
}
void StudioController::fetch(QUrl url,std::function<void(QByteArray)> done) {
    if(url.isLocalFile()) {
        QFile f(url.toLocalFile());if(!f.open(QIODevice::ReadOnly)||f.size()>64*1024*1024){report("Cannot read file or it exceeds 64 MB.");return;}
        done(f.readAll());return;
    }
    if(url.scheme()!="https"){report("Catalogs and packages must use HTTPS or a local file.");return;}
    QNetworkRequest request(url);request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply=m_network->get(request);auto bytes=std::make_shared<QByteArray>();
    connect(reply,&QIODevice::readyRead,this,[reply,bytes]{
        if(bytes->size()+reply->bytesAvailable()>64*1024*1024){reply->abort();return;}bytes->append(reply->readAll());
    });
    connect(reply,&QNetworkReply::finished,this,[this,reply,bytes,done]{
        const auto error=reply->error();bytes->append(reply->readAll());reply->deleteLater();
        if(error!=QNetworkReply::NoError||bytes->size()>64*1024*1024){report("Download failed. Check the address, connection and package size.");return;}
        done(*bytes);
    });
}
void StudioController::loadCatalog(const QUrl &url) {
    report("Loading catalog…");
    fetch(url,[this,url](QByteArray bytes){
        const auto object=QJsonDocument::fromJson(bytes).object();const auto entries=object.value("plugins").toArray();
        if(object.value("format").toString()!="andiya-catalog"||entries.size()>500){report("Invalid catalog.");return;}
        QVariantList next;
        for(const auto &value:entries){
            auto item=value.toObject().toVariantMap();const auto package=url.resolved(QUrl(item.value("package").toString()));
            if(!package.isValid()||(!package.isLocalFile()&&package.scheme()!="https"))continue;
            item["package"]=package.toString();next<<item;
        }
        m_catalog=next;emit catalogChanged();report("Catalog loaded. Publisher identity is checked on the downloaded package.");
    });
}
void StudioController::installCatalogItem(int index) {
    if(index<0||index>=m_catalog.size())return;
    report("Downloading plugin…");fetch(QUrl(m_catalog[index].toMap().value("package").toString()),[this](QByteArray bytes){unpack(bytes);});
}
void StudioController::installBundle(const QUrl &url) {fetch(url,[this](QByteArray bytes){unpack(bytes);});}
void StudioController::unpack(const QByteArray &bytes) {
    const auto object=QJsonDocument::fromJson(bytes).object();const auto files=object.value("files").toObject();
    if(object.value("format").toString()!="andiya-plugin-bundle"||files.isEmpty()||files.size()>2000){report("Invalid plugin bundle.");return;}
    QTemporaryDir staging;if(!staging.isValid()){report("Cannot create a staging folder.");return;}
    static const QRegularExpression safe("^[A-Za-z0-9_.-]+$");
    for(auto it=files.begin();it!=files.end();++it) {
        const auto parts=it.key().split('/');
        for(const auto &part:parts)if(part=="."||part==".."||part.endsWith('.')||!safe.match(part).hasMatch()){
            report("Package contains an unsafe file path.");return;
        }
        const QString path=staging.path()+"/"+it.key();
        QDir().mkpath(QFileInfo(path).absolutePath());QFile file(path);
        const auto decoded=QByteArray::fromBase64Encoding(it.value().toString().toLatin1(),QByteArray::AbortOnBase64DecodingErrors);
        if(!decoded||!file.open(QIODevice::WriteOnly)||file.write(decoded.decoded)!=decoded.decoded.size()){report("Cannot unpack plugin file.");return;}
    }
    const auto error=m_plugins->installPlugin(QUrl::fromLocalFile(staging.path()));
    report(error.isEmpty()?"Plugin installed. Review its publisher and permissions before enabling.":error);
}
