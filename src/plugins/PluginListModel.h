#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QImage>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include "PluginValidation.h"
#include "PythonImageFilterHost.h"

class PluginListModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QVariantList details READ details NOTIFY filterStackChanged)
    Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY presetsChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        VendorRole,
        DescriptionRole,
        CapabilityRole,
        AccentRole,
        EnabledRole,
        OnlineRole,
        BuiltInRole,
        AvailableRole,
        ErrorRole
    };

    explicit PluginListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void setPluginEnabled(int index, bool enabled);
    Q_INVOKABLE QString installPlugin(const QUrl &directoryUrl);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void approvePlugin(const QString &id,const QString &fingerprint);
    QVariantList details() const;
    QVariantList snapshot() const;
    QStringList presetNames() const;
    Q_INVOKABLE void setStrength(int index, double value);
    Q_INVOKABLE void setParameter(int index, const QString &key, double value);
    Q_INVOKABLE void savePreset(const QString &name);
    Q_INVOKABLE void loadPreset(const QString &name);
    Q_INVOKABLE void deletePreset(const QString &name);
    Q_INVOKABLE QString rollbackPlugin(const QString &id);
    void saveState() const;
    // Moves the filter at `index` earlier/later in the pipeline. Only
    // affects image filters (native or Python); the new order is applied
    // immediately to the processing snapshot. The order is persisted.
    Q_INVOKABLE bool moveFilterUp(int index);
    Q_INVOKABLE bool moveFilterDown(int index);


    [[nodiscard]] bool hasEnabledImageFilters() const;

signals:
    void filterStackChanged();
    void presetsChanged();
    void approvalRequired(const QString &id,const QString &fingerprint,const QString &name,const QString &details);
    void maintenanceRequested();

private:
    struct PluginEntry {
        QString id;
        QString name;
        QString vendor;
        QString description;
        QString capability;
        QString accent;
        bool enabled = false;
        bool online = false;
        bool builtIn = false;
        bool available = false;
        bool imageFilter = false;
        bool pythonFilter = false;
        QString error;
        QString path;
        QVariantMap manifest;
        QVariantMap parameters;
        double strength = 1.0;
    };

    void discoverDirectory(const QString &path);
    static bool copyPluginDirectory(const QString &sourcePath,
                                    const QString &destinationPath,
                                    QString *errorMessage);

    QVector<PluginEntry> m_plugins;

};
