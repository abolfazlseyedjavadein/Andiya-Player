#pragma once
#include <QObject>
#include <functional>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
class PlayerController;class PluginListModel;class QNetworkAccessManager;
class StudioController final:public QObject {
    Q_OBJECT
    Q_PROPERTY(bool secureProfilesAvailable READ secureProfilesAvailable CONSTANT)
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY profilesChanged)
    Q_PROPERTY(QVariantList catalog READ catalog NOTIFY catalogChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(double textScale READ textScale WRITE setTextScale NOTIFY preferencesChanged)
    Q_PROPERTY(QVariantMap shortcuts READ shortcuts NOTIFY preferencesChanged)
public:
    StudioController(PlayerController *player,PluginListModel *plugins,QObject *parent=nullptr);
    static QUrl publicUrl(QUrl url);
    bool secureProfilesAvailable() const {
#ifdef Q_OS_WIN
        return true;
#else
        return false;
#endif
    }
    QVariantList profiles()const;
    QVariantList catalog()const{return m_catalog;}
    QString status()const{return m_status;}
    double textScale()const;
    QVariantMap shortcuts()const;
    void setTextScale(double value);
    Q_INVOKABLE QString setShortcut(const QString &action,const QString &sequence);
    Q_INVOKABLE void resetShortcuts();
    Q_INVOKABLE void saveProfile(const QString &name,const QString &url);
    Q_INVOKABLE void openProfile(int index);
    Q_INVOKABLE void deleteProfile(int index);
    Q_INVOKABLE void exportWorkspace(const QUrl &file);
    Q_INVOKABLE void importWorkspace(const QUrl &file);
    Q_INVOKABLE void loadCatalog(const QUrl &url);
    Q_INVOKABLE void installCatalogItem(int index);
    Q_INVOKABLE void installBundle(const QUrl &file);
signals:
    void profilesChanged();
    void catalogChanged();
    void statusChanged();
    void preferencesChanged();
private:
    void report(QString text);
    void fetch(QUrl url,std::function<void(QByteArray)> done);
    void unpack(const QByteArray &bytes);
    PlayerController *m_player;
    PluginListModel *m_plugins;
    QNetworkAccessManager *m_network;
    QVariantList m_catalog;
    QString m_status="Ready";
};
