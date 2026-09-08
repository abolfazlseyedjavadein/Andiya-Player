#include "PluginListModel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QUuid>
#include <QSettings>
#include <algorithm>
#include <cmath>

#include <utility>

PluginListModel::PluginListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    reload();
}

int PluginListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_plugins.size();
}

QVariant PluginListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_plugins.size()) {
        return {};
    }

    const PluginEntry &plugin = m_plugins.at(index.row());
    switch (role) {
    case IdRole: return plugin.id;
    case NameRole: return plugin.name;
    case VendorRole: return plugin.vendor;
    case DescriptionRole: return plugin.description;
    case CapabilityRole: return plugin.capability;
    case AccentRole: return plugin.accent;
    case EnabledRole: return plugin.enabled;
    case OnlineRole: return plugin.online;
    case BuiltInRole: return plugin.builtIn;
    case AvailableRole: return plugin.available;
    case ErrorRole: return plugin.error;
    default: return {};
    }
}

QHash<int, QByteArray> PluginListModel::roleNames() const
{
    return {
        {IdRole, "pluginId"},
        {NameRole, "name"},
        {VendorRole, "vendor"},
        {DescriptionRole, "description"},
        {CapabilityRole, "capability"},
        {AccentRole, "accent"},
        {EnabledRole, "pluginEnabled"},
        {OnlineRole, "online"},
        {BuiltInRole, "builtIn"},
        {AvailableRole, "available"},
        {ErrorRole, "loadError"}
    };
}

void PluginListModel::setPluginEnabled(int index, bool enabled)
{
    if(index<0||index>=m_plugins.size()||!m_plugins[index].available)return;
    if(enabled && QSettings().value("plugins/approved/"+m_plugins[index].id).toString()!=m_plugins[index].manifest.value("fingerprint").toString()){
        const auto &p=m_plugins[index];
        emit approvalRequired(p.id,p.manifest.value("fingerprint").toString(),p.name,p.manifest.value("publisherStatus").toString()+"\nRequested access: "+p.manifest.value("permissions").toStringList().join(", ")+"\nPlugins run in separate processes with your account's access. Declared permissions are informational, not an operating-system sandbox.");
        return;
    }
    if(enabled && m_plugins[index].enabled)emit maintenanceRequested();
    m_plugins[index].enabled=enabled;
    emit dataChanged(createIndex(index,0),createIndex(index,0),{EnabledRole});
    saveState();emit filterStackChanged();
}

bool PluginListModel::moveFilterUp(int index)
{
    if (index <= 0 || index >= m_plugins.size()) {
        return false;
    }
    beginMoveRows(QModelIndex{}, index, index, QModelIndex{}, index - 1);
    m_plugins.move(index, index - 1);
    endMoveRows();
    saveState();
    emit filterStackChanged();
    return true;
}

bool PluginListModel::moveFilterDown(int index)
{
    if (index < 0 || index + 1 >= m_plugins.size()) {
        return false;
    }
    beginMoveRows(QModelIndex{}, index, index, QModelIndex{}, index + 2);
    m_plugins.move(index, index + 1);
    endMoveRows();
    saveState();
    emit filterStackChanged();
    return true;
}

QString PluginListModel::installPlugin(const QUrl &directoryUrl)
{
    if (!directoryUrl.isLocalFile()) {
        return QStringLiteral("Choose a local plugin folder.");
    }

    const auto validation = PluginValidation::inspect(directoryUrl.toLocalFile());
    if (!validation.value("valid").toBool()) return validation.value("error").toString();
    const QDir source(directoryUrl.toLocalFile());
    if (!source.exists()) {
        return QStringLiteral("The selected plugin folder does not exist.");
    }

    QFile manifest(source.filePath(QStringLiteral("plugin.json")));
    if (!manifest.open(QIODevice::ReadOnly)) {
        return QStringLiteral("The selected folder does not contain plugin.json.");
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(manifest.readAll(), &parseError);
    if (!document.isObject()) {
        return QStringLiteral("plugin.json is invalid: %1").arg(parseError.errorString());
    }

    const QString id = document.object().value(QStringLiteral("id")).toString();
    static const QRegularExpression safeId(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$"));
    if (!safeId.match(id).hasMatch()) {
        return QStringLiteral("The plugin id is missing or contains unsupported characters.");
    }


    const QString pluginRoot = QDir(QStandardPaths::writableLocation(
                                        QStandardPaths::AppDataLocation))
                                   .filePath(QStringLiteral("plugins"));
    if (!QDir().mkpath(pluginRoot)) {
        return QStringLiteral("Andiya could not create its user plugin directory.");
    }

    const QString destination = QDir(pluginRoot).filePath(id);

    const QString temporary = QDir(pluginRoot).filePath(
        QStringLiteral(".installing-%1-%2")
            .arg(id, QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QString copyError;
    if (!copyPluginDirectory(source.absolutePath(), temporary, &copyError)) {
        QDir(temporary).removeRecursively();
        return copyError;
    }

    const auto copiedValidation=PluginValidation::inspect(temporary);
    if(!copiedValidation.value("valid").toBool()){
        QDir(temporary).removeRecursively();return copiedValidation.value("error").toString();
    }
    emit maintenanceRequested();
    const QString backupRoot = QDir(pluginRoot).filePath(".rollback");
    QDir().mkpath(backupRoot);
    const QString backup = QDir(backupRoot).filePath(id);
    if (QFileInfo::exists(destination)) {
        if (QFileInfo::exists(backup) && !QDir(backup).removeRecursively()) {
            QDir(temporary).removeRecursively();
            return "Could not replace the previous rollback copy.";
        }
        if (!QDir().rename(destination, backup)) { QDir(temporary).removeRecursively(); return "Could not retain the previous plugin version."; }
    }
    if (!QDir().rename(temporary, destination)) {
        if (QFileInfo::exists(backup)) QDir().rename(backup, destination);
        QDir(temporary).removeRecursively();
        return QStringLiteral("Andiya could not finish installing the plugin.");
    }

    reload();
    return {};
}

void PluginListModel::reload()
{

    beginResetModel();
    m_plugins.clear();

    discoverDirectory(QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                          .filePath(QStringLiteral("plugins")));
    discoverDirectory(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("plugins")));
    const auto saved = QSettings().value("plugins/stack").toList();
    for (const auto &value : saved) {
        const auto entry = value.toMap();
        for (auto &plugin : m_plugins) if (plugin.id == entry.value("id").toString()) {
            plugin.enabled = plugin.available && entry.value("enabled").toBool() &&
                QSettings().value("plugins/approved/"+plugin.id).toString()==plugin.manifest.value("fingerprint").toString();
            plugin.strength = qBound(0.0, entry.value("strength", 1.0).toDouble(), 1.0);
            const auto savedParameters=entry.value("parameters").toMap();
            for(const auto &value:plugin.manifest.value("parameters").toList()){
                const auto spec=value.toMap();const auto key=spec.value("key").toString();
                bool ok=false;const double number=savedParameters.value(key).toDouble(&ok);
                if(ok&&std::isfinite(number))plugin.parameters[key]=qBound(spec.value("min",0).toDouble(),number,spec.value("max",1).toDouble());
            }
        }
    }
    QStringList order;
    for (const auto &value : saved) order << value.toMap().value("id").toString();
    std::stable_sort(m_plugins.begin(), m_plugins.end(), [&order](const auto &a, const auto &b) {
        const int ai = order.indexOf(a.id), bi = order.indexOf(b.id);
        return (ai < 0 ? 100000 : ai) < (bi < 0 ? 100000 : bi);
    });
    endResetModel();
    saveState();
    emit filterStackChanged();
}

bool PluginListModel::hasEnabledImageFilters() const
{
    for (const auto &p : m_plugins) if (p.enabled && p.available && p.imageFilter) return true;
    return false;
}

bool PluginListModel::copyPluginDirectory(const QString &sourcePath,
                                          const QString &destinationPath,
                                          QString *errorMessage)
{
    const QDir source(sourcePath);
    if (!QDir().mkpath(destinationPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not create the plugin destination folder.");
        }
        return false;
    }

    const QFileInfoList entries = source.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    for (const QFileInfo &entry : entries) {
        const QString destination = QDir(destinationPath).filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyPluginDirectory(entry.absoluteFilePath(), destination, errorMessage)) {
                return false;
            }
        } else if (!QFile::copy(entry.absoluteFilePath(), destination)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Could not copy plugin file: %1").arg(entry.fileName());
            }
            return false;
        }
    }
    return true;
}

void PluginListModel::discoverDirectory(const QString &path)
{
    const QDir root(path);
    for(const auto &directory:root.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::NoSymLinks)) {
        if(directory.fileName().startsWith('.'))continue;
        const auto report=PluginValidation::inspect(directory.absoluteFilePath());
        const auto manifest=report.value("manifest").toMap();
        QString id=manifest.value("id",directory.fileName()).toString();
        if(std::any_of(m_plugins.begin(),m_plugins.end(),[&](const auto &p){return p.id==id;}))continue;
        PluginEntry p;
        p.id=id;p.name=manifest.value("name",id).toString();p.vendor=manifest.value("vendor","Third party").toString();
        p.description=manifest.value("description").toString();p.capability="IMAGE FILTER";
        p.accent=manifest.value("accent","#5CD6FF").toString();p.available=report.value("valid").toBool();
        p.imageFilter=true;p.pythonFilter=manifest.value("execution").toString()=="python";
        p.error=report.value("error").toString();p.path=report.value("path").toString();p.manifest=manifest;
        p.manifest["publisherStatus"]=report.value("publisherStatus");
        p.manifest["fingerprint"]=report.value("fingerprint");
        for(const auto &value:manifest.value("parameters").toList()) {
            const auto spec=value.toMap();p.parameters[spec.value("key").toString()]=spec.value("default");
        }
        // Discovery never executes a plugin. Enabling is an explicit user decision.
        m_plugins.push_back(p);
    }
}

QVariantList PluginListModel::details() const {
    QVariantList result;
    for (int i = 0; i < m_plugins.size(); ++i) {
        const auto &p = m_plugins[i];
        auto info = p.manifest;
        info["index"] = i; info["enabled"] = p.enabled; info["available"] = p.available;
        info["strength"] = p.strength; info["values"] = p.parameters; info["error"] = p.error;

        result << info;
    }
    return result;
}
QVariantList PluginListModel::snapshot() const {
    QVariantList result;
    for (const auto &p : m_plugins) if (p.enabled && p.available && p.imageFilter)
        result << QVariantMap{{"id", p.id}, {"path", p.path}, {"native", !p.pythonFilter},
                              {"parameters", p.parameters}, {"strength", p.strength}};
    return result;
}
void PluginListModel::saveState() const {
    QVariantList result;
    for (const auto &p : m_plugins) result << QVariantMap{{"id", p.id}, {"enabled", p.enabled},
                                        {"parameters", p.parameters}, {"strength", p.strength}};
    QSettings().setValue("plugins/stack", result);
}
void PluginListModel::setStrength(int index, double value) {
    if (index < 0 || index >= m_plugins.size() || !std::isfinite(value)) return;
    m_plugins[index].strength = qBound(0.0, value, 1.0); saveState(); emit filterStackChanged();
}
void PluginListModel::setParameter(int index, const QString &key, double value) {
    if (index < 0 || index >= m_plugins.size() || !std::isfinite(value)) return;
    for (const auto &item : m_plugins[index].manifest.value("parameters").toList()) {
        const auto spec = item.toMap();
        if (spec.value("key").toString() != key) continue;
        const double min = spec.value("min", 0).toDouble(), max = spec.value("max", 1).toDouble();
        if (min > max) return;
        m_plugins[index].parameters[key] = qBound(min, value, max);
        saveState(); emit filterStackChanged(); return;
    }
}
QStringList PluginListModel::presetNames() const { return QSettings().value("plugins/presets").toMap().keys(); }
void PluginListModel::savePreset(const QString &name) {
    if (name.trimmed().isEmpty() || name.size() > 100) return;
    saveState(); QSettings settings; auto presets = settings.value("plugins/presets").toMap();
    presets[name.trimmed()] = settings.value("plugins/stack");
    settings.setValue("plugins/presets", presets); emit presetsChanged();
}
void PluginListModel::loadPreset(const QString &name) {
    QSettings settings; const auto presets = settings.value("plugins/presets").toMap();
    if (!presets.contains(name)) return;
    settings.setValue("plugins/stack", presets.value(name)); reload();
}
void PluginListModel::deletePreset(const QString &name) {
    QSettings settings; auto presets = settings.value("plugins/presets").toMap();
    presets.remove(name); settings.setValue("plugins/presets", presets); emit presetsChanged();
}
QString PluginListModel::rollbackPlugin(const QString &id) {
    static const QRegularExpression safe("^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$");
    if (!safe.match(id).hasMatch()) return "Invalid plugin ID.";
    const QDir root(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/plugins");
    const QString current = root.filePath(id), previous = root.filePath(".rollback/" + id);
    const QString swap = root.filePath(".rollback/swap-" + QUuid::createUuid().toString(QUuid::WithoutBraces));
    if (!QFileInfo::exists(previous)) return "No previous local version is available.";
    emit maintenanceRequested();
    if (!QDir().rename(current, swap)) return "Could not retain the current version.";
    if (!QDir().rename(previous, current)) { QDir().rename(swap, current); return "Rollback failed."; }
    if (!QDir().rename(swap, previous)) return "Restored the prior version; could not retain the newer version.";
    reload(); return {};
}

void PluginListModel::approvePlugin(const QString &id,const QString &fingerprint) {
    for(int index=0;index<m_plugins.size();++index){
        const auto &p=m_plugins[index];if(p.id!=id||!p.available||p.manifest.value("fingerprint").toString()!=fingerprint)continue;
        QSettings().setValue("plugins/approved/"+p.id,fingerprint);setPluginEnabled(index,true);return;
    }
}
