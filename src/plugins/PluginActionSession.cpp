#include "PluginActionSession.h"
#include "PythonImageFilterHost.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent/QtConcurrentRun>
#include <cmath>

PluginActionSession::PluginActionSession(QObject *parent) : QObject(parent)
{
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(4000);
    connect(&m_timeout, &QTimer::timeout, this, &PluginActionSession::fail);
}

void PluginActionSession::start(const QString &script, const QVariantMap &parameters,
                                const QString &actionId, bool rectangle)
{
    stop();
    m_rectangle = rectangle;
    const auto generation = m_generation;
    auto *interpreter = new QFutureWatcher<QString>(this);
    connect(interpreter, &QFutureWatcher<QString>::finished, this, [this, interpreter, generation, script, parameters, actionId] {
        const auto executable = interpreter->result();
        interpreter->deleteLater();
        if (generation != m_generation) return;
        if (executable.isEmpty() || !QFileInfo(script).isFile()) {
            fail();
            return;
        }

        auto *process = new QProcess(this);
        m_process = process;
        process->setWorkingDirectory(QFileInfo(script).absolutePath());
        process->setStandardErrorFile(QProcess::nullDevice());
        connect(process, &QProcess::started, this, &PluginActionSession::sendNext);
        connect(process, &QProcess::readyReadStandardOutput, this, [this, process] { receive(process); });
        connect(process, &QProcess::errorOccurred, this, [this, process] {
            if (m_process == process) fail();
        });
        connect(process, &QProcess::finished, this, [this, process] {
            if (m_process == process) fail();
        });
        const QString runner = QCoreApplication::applicationDirPath() + "/python_runtime/andiya_python_host.py";
        const auto json = QJsonDocument::fromVariant(parameters).toJson(QJsonDocument::Compact);
        QStringList arguments{runner, script, QString::fromUtf8(json)};
        if (actionId.isEmpty()) arguments << "--rectangle-editor";
        else arguments << "--plugin-action" << actionId;
        process->start(executable, arguments);
        if (m_process == process) m_timeout.start();
    });
    interpreter->setFuture(QtConcurrent::run(&PythonImageFilterHost::findInterpreter));
}

void PluginActionSession::request(const QVariantMap &event)
{
    m_pending = QJsonDocument::fromVariant(event).toJson(QJsonDocument::Compact) + '\n';
    emit busyChanged();
    sendNext();
}

void PluginActionSession::sendNext()
{
    if (!m_process || m_process->state() != QProcess::Running || m_waiting || m_pending.isEmpty())
        return;
    m_waiting = true;
    if (m_process->write(m_pending) != m_pending.size()) {
        fail();
        return;
    }
    m_pending.clear();
    m_timeout.start();
}

void PluginActionSession::receive(QProcess *process)
{
    if (m_process != process) return;
    m_response += process->readAllStandardOutput();
    if (m_response.size() > 4096) {
        fail();
        return;
    }
    if (!m_response.endsWith('\n')) return;

    const auto response = QJsonDocument::fromJson(m_response);
    m_response.clear();
    if (!m_waiting) {
        fail();
        return;
    }
    if (!m_rectangle) {
        if (!response.isObject()) {
            fail();
            return;
        }
        const auto updates = response.object();
        for (const auto &value : updates) {
            if (!value.isDouble() || !std::isfinite(value.toDouble())) {
                fail();
                return;
            }
        }
        stop();
        emit parametersReturned(updates.toVariantMap());
        return;
    }

    const auto values = response.array();
    if (values.size() != 4) {
        fail();
        return;
    }
    for (const auto &value : values) {
        const double number = value.toDouble(-1);
        if (!std::isfinite(number) || number < 0 || number > 1) {
            fail();
            return;
        }
    }
    const QRectF region(values[0].toDouble(), values[1].toDouble(), values[2].toDouble(), values[3].toDouble());
    if (region.isEmpty() || region.right() > 1.000000001 || region.bottom() > 1.000000001) {
        fail();
        return;
    }

    m_timeout.stop();
    m_waiting = false;
    emit selectionChanged(region);
    sendNext();
    emit busyChanged();
}

void PluginActionSession::stop()
{
    ++m_generation;
    m_timeout.stop();
    m_pending.clear();
    m_response.clear();
    m_waiting = false;
    if (auto *process = m_process.data()) {
        m_process.clear();
        process->kill();
        process->deleteLater();
    }
    emit busyChanged();
}

void PluginActionSession::fail()
{
    stop();
    emit failed();
}
