#include "PluginValidation.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QSet>
#include <cmath>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <wincrypt.h>
#endif

QVariantMap PluginValidation::inspect(const QString &directory) {
    QVariantMap result{{"valid",false},{"publisherStatus","Unsigned — publisher not verified"}};
    auto fail=[&](QString message){result["error"]=message;return result;};
    const QDir root(directory); QFile file(root.filePath("plugin.json"));
    if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024) return fail("Missing or oversized plugin.json.");
    const QByteArray bytes=file.readAll(); const auto doc=QJsonDocument::fromJson(bytes);
    if(!doc.isObject()) return fail("Invalid manifest JSON.");
    const auto obj=doc.object(); const auto id=obj.value("id").toString();
    if(!QRegularExpression("^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$").match(id).hasMatch()) return fail("Invalid plugin ID.");
    if(obj.value("andiyaApi").toString()!="1.0") return fail("This plugin requires a different API version.");
    if(!QRegularExpression("^\\d+\\.\\d+\\.\\d+([+-][A-Za-z0-9.-]+)?$").match(obj.value("version").toString()).hasMatch())
        return fail("Version must use major.minor.patch.");
    QSet<QString> parameterKeys;
    const auto specs=obj.value("parameters").toArray();
    if(specs.size()>32)return fail("Too many parameters.");
    for(const auto &value:specs) {
        const auto spec=value.toObject();const auto key=spec.value("key").toString();
        const double min=spec.value("min").toDouble(0),max=spec.value("max").toDouble(1),def=spec.value("default").toDouble();
        if(!QRegularExpression("^[A-Za-z][A-Za-z0-9_]{0,63}$").match(key).hasMatch()||parameterKeys.contains(key)||
           !std::isfinite(min)||!std::isfinite(max)||min>=max||def<min||def>max)return fail("Invalid numeric parameter schema.");
        parameterKeys.insert(key);
    }
    const auto execution=obj.value("execution").toString();
    if(execution!="native" && execution!="python") return fail("Supported execution types: native and python.");
    const auto runtime=obj.value("runtime").toString();
    if((execution=="native" && runtime!="c-abi") || (execution=="python" && runtime!="process" && runtime!="python"))
        return fail("Incompatible runtime.");
    const QString entry=obj.value("entrypoint").toString();
    const auto entryFile=QFileInfo(root.filePath(entry));
    const auto boundary=QFileInfo(directory).canonicalFilePath()+"/";
#ifdef Q_OS_WIN
    constexpr auto pathCase = Qt::CaseInsensitive;
#else
    constexpr auto pathCase = Qt::CaseSensitive;
#endif
    if(entry.isEmpty() || QDir::isAbsolutePath(entry) || !entryFile.isFile() ||
       !entryFile.canonicalFilePath().startsWith(boundary,pathCase)) return fail("Entrypoint must be a file inside this folder.");
    const auto hashes=obj.value("files").toObject();
    QCryptographicHash packageHash(QCryptographicHash::Sha256);packageHash.addData(bytes);
    quint64 total=0; int count=0;
    QDirIterator it(directory,QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot|QDir::Hidden,QDirIterator::Subdirectories);
    QStringList paths;while(it.hasNext())paths<<it.next();paths.sort(Qt::CaseSensitive);
    for(const auto &path:paths) {
        const QFileInfo info(path);
        if(info.isSymLink() || !info.canonicalFilePath().startsWith(boundary,pathCase)) return fail("Links and escaped paths are not allowed.");
        if(info.isDir()) continue;
        if(++count>2000 || (total+=info.size())>512ULL*1024*1024) return fail("Plugin exceeds package limits.");
        const auto relative=root.relativeFilePath(info.filePath());
        if(relative=="plugin.json"||relative=="plugin.json.p7s") continue;
        QFile contentHashFile(info.filePath());if(!contentHashFile.open(QIODevice::ReadOnly))return fail("Cannot read package file.");
        QCryptographicHash contentHash(QCryptographicHash::Sha256);contentHash.addData(&contentHashFile);
        packageHash.addData(relative.toUtf8());packageHash.addData(contentHash.result());
        if(!hashes.isEmpty()) {
            QFile content(info.filePath()); if(!content.open(QIODevice::ReadOnly)) return fail("Cannot read "+relative);
            QCryptographicHash hash(QCryptographicHash::Sha256);
            if(!hash.addData(&content) || hash.result().toHex()!=hashes.value(relative).toString().toLatin1().toLower())
                return fail("Missing or mismatched SHA-256: "+relative);
        }
    }
    for(auto it=hashes.begin();it!=hashes.end();++it) {
        const QFileInfo f(root.filePath(it.key()));
        if(!f.isFile()||!f.canonicalFilePath().startsWith(boundary,pathCase)) return fail("Invalid hashed file: "+it.key());
    }
    QFile signature(root.filePath("plugin.json.p7s"));
    if(signature.exists()) {
        if(hashes.isEmpty()||!signature.open(QIODevice::ReadOnly)||signature.size()>1024*1024) return fail("Signed plugins require file hashes and a readable signature.");
#ifdef Q_OS_WIN
        auto sig=signature.readAll();
        CRYPT_VERIFY_MESSAGE_PARA para{};para.cbSize=sizeof(para);para.dwMsgAndCertEncodingType=X509_ASN_ENCODING|PKCS_7_ASN_ENCODING;
        const BYTE *data=reinterpret_cast<const BYTE *>(bytes.constData()); DWORD length=DWORD(bytes.size());
        PCCERT_CONTEXT signer=nullptr;
        if(!CryptVerifyDetachedMessageSignature(&para,0,reinterpret_cast<const BYTE *>(sig.constData()),DWORD(sig.size()),1,&data,&length,&signer))
            return fail("Publisher signature is invalid.");
        CERT_CHAIN_PARA chainPara{};chainPara.cbSize=sizeof(chainPara); PCCERT_CHAIN_CONTEXT chain=nullptr;
        const bool built=CertGetCertificateChain(nullptr,signer,nullptr,signer->hCertStore,&chainPara,CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL,nullptr,&chain);
        CERT_CHAIN_POLICY_PARA policy{};policy.cbSize=sizeof(policy);
        CERT_CHAIN_POLICY_STATUS status{};status.cbSize=sizeof(status);
        const bool trusted=built && CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_BASE,chain,&policy,&status) && !status.dwError;
        wchar_t name[512]{};CertGetNameStringW(signer,CERT_NAME_SIMPLE_DISPLAY_TYPE,0,nullptr,name,512);
        result["publisherStatus"]=trusted ? "Signature verified: "+QString::fromWCharArray(name)
                                          : "Valid signature; certificate not trusted: "+QString::fromWCharArray(name);
        result["trustedPublisher"]=trusted;
        if(chain)CertFreeCertificateChain(chain);CertFreeCertificateContext(signer);
#else
        return fail("Signature verification is currently available on Windows.");
#endif
    }
    result["fingerprint"]=QString::fromLatin1(packageHash.result().toHex());
    result["valid"]=true;result["error"]="";result["manifest"]=obj.toVariantMap();
    result["path"]=entryFile.canonicalFilePath(); return result;
}
