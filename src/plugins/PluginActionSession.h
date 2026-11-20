#pragma once

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QRectF>
#include <QTimer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class PluginActionSession : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit PluginActionSession(QObject *parent = nullptr);

    bool busy() const { return m_waiting || !m_pending.isEmpty(); }
    Q_INVOKABLE void start(const QString &script, const QVariantMap &parameters,
                           const QString &actionId, bool rectangle);
    Q_INVOKABLE void request(const QVariantMap &event);
    Q_INVOKABLE void stop();

signals:
    void busyChanged();
    void selectionChanged(const QRectF &selection);
    void parametersReturned(const QVariantMap &parameters);
    void failed();

private:
    void sendNext();
    void receive(QProcess *process);
    void fail();

    QPointer<QProcess> m_process;
    QTimer m_timeout;
    QByteArray m_response;
    QByteArray m_pending;
    quint64 m_generation = 0;
    bool m_waiting = false;
    bool m_rectangle = true;
};
