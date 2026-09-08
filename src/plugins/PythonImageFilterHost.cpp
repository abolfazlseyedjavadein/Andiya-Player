#include "PythonImageFilterHost.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QElapsedTimer>

#include <cstring>

namespace {
constexpr quint32 kRequestMagic = 0x414E4652u;  // "ANFR"
constexpr quint32 kResponseMagic = 0x414E5253u; // "ANRS"
// "<IIIIIqQ" in the Python struct module: 5 * uint32 + int64 + uint64.
constexpr int kRequestHeaderSize = 4 * 5 + 8 + 8;
// "<IiIIIIQ": magic, status, width, height, stride, pixel_format, data_size.
constexpr int kResponseHeaderSize = 4 * 6 + 8;
constexpr int kStartTimeoutMs = 4000;
constexpr int kWriteTimeoutMs = 3000;
constexpr int kReadTimeoutMs = 5000;

// Blocks until exactly `size` bytes have been read into `buffer` (appended),
// or returns false on timeout/EOF. This I/O runs on the dedicated filter thread.
bool readExact(QProcess *process, qint64 size, QByteArray &buffer)
{
    QElapsedTimer deadline; deadline.start();
    while (buffer.size() < size) {
        if (deadline.elapsed() >= kReadTimeoutMs) return false;
        if (process->bytesAvailable() <= 0) {
            if (!process->waitForReadyRead(kReadTimeoutMs - int(deadline.elapsed()))) {
                return false;
            }
        }
        buffer += process->read(size - buffer.size());
    }
    return true;
}
}

PythonImageFilterHost::PythonImageFilterHost() = default;

PythonImageFilterHost::~PythonImageFilterHost()
{
    clear();
}

QString PythonImageFilterHost::findInterpreter()
{
    // "py" (the official Python launcher) is checked first on Windows
    // because a bare "python"/"python3" often resolves to Microsoft Store's
    // app-execution-alias stub, which "exists" on PATH but exits with an
    // error instead of running anything unless Python was installed through
    // the Store. We verify each candidate actually runs `--version`
    // successfully rather than trusting PATH resolution alone.
#if defined(Q_OS_WIN)
    static const QStringList candidates = {
        QStringLiteral("py"), QStringLiteral("python3"), QStringLiteral("python")};
#else
    static const QStringList candidates = {
        QStringLiteral("python3"), QStringLiteral("python")};
#endif
    for (const QString &candidate : candidates) {
        const QString resolved = QStandardPaths::findExecutable(candidate);
        if (resolved.isEmpty()) {
            continue;
        }
        QProcess probe;
        probe.start(resolved, {QStringLiteral("--version")});
        if (probe.waitForFinished(2000) && probe.exitStatus() == QProcess::NormalExit &&
            probe.exitCode() == 0) {
            return resolved;
        }
    }
    return {};
}

bool PythonImageFilterHost::load(const QString &manifestId, const QString &scriptPath,
                                 QString *errorMessage)
{
    if (isLoaded(manifestId)) {
        return true;
    }

    if (!QFileInfo::exists(scriptPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Python plugin script not found: %1").arg(scriptPath);
        }
        return false;
    }

    if (m_interpreter.isEmpty()) {
        m_interpreter = findInterpreter();
    }
    if (m_interpreter.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "No Python 3 interpreter found on PATH (looked for python3, python, py).");
        }
        return false;
    }

    const QString runnerPath = QDir(QCoreApplication::applicationDirPath())
                                   .filePath(QStringLiteral("python_runtime/andiya_python_host.py"));
    if (!QFileInfo::exists(runnerPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Andiya's Python bridge script is missing: %1")
                                .arg(runnerPath);
        }
        return false;
    }

    LoadedFilter filter;
    filter.id = manifestId;
    filter.scriptPath = scriptPath;
    m_filters.push_back(std::move(filter));
    return true;
}

void PythonImageFilterHost::setEnabled(const QString &id, bool enabled)
{
    for (LoadedFilter &filter : m_filters) {
        if (filter.id == id) {
            filter.enabled = enabled;
            if (!enabled && filter.process) { filter.process->kill(); filter.process->waitForFinished(1000); filter.process.reset(); }
            if (enabled) filter.failed = false;
            return;
        }
    }
}

void PythonImageFilterHost::clear()
{
    for (LoadedFilter &filter : m_filters) {
        if (filter.process) {
            filter.process->closeWriteChannel();
            if (!filter.process->waitForFinished(500)) {
                filter.process->kill();
                filter.process->waitForFinished(500);
            }
        }
    }
    m_filters.clear();
}

bool PythonImageFilterHost::isLoaded(const QString &id) const
{
    for (const LoadedFilter &filter : m_filters) {
        if (filter.id == id) {
            return true;
        }
    }
    return false;
}

bool PythonImageFilterHost::isEnabled(const QString &id) const
{
    for (const LoadedFilter &filter : m_filters) {
        if (filter.id == id) {
            return filter.enabled && !filter.failed;
        }
    }
    return false;
}

bool PythonImageFilterHost::hasEnabledFilters() const
{
    for (const LoadedFilter &filter : m_filters) {
        if (filter.enabled && !filter.failed) {
            return true;
        }
    }
    return false;
}

bool PythonImageFilterHost::ensureStarted(LoadedFilter &filter) const
{
    if (filter.process && filter.process->state() == QProcess::Running) {
        return true;
    }
    filter.process.reset();

    const QString runnerPath = QDir(QCoreApplication::applicationDirPath())
                                   .filePath(QStringLiteral("python_runtime/andiya_python_host.py"));

    auto process = std::make_unique<QProcess>();
    process->setProgram(filter.native ? QCoreApplication::applicationDirPath() + QStringLiteral("/AndiyaFilterWorker")
#ifdef Q_OS_WIN
        + QStringLiteral(".exe")
#endif
 : m_interpreter);
    process->setArguments(filter.native ? QStringList{filter.id, filter.scriptPath, filter.parameters}
                                        : QStringList{runnerPath, filter.scriptPath, filter.parameters});
    process->setWorkingDirectory(QFileInfo(filter.scriptPath).absolutePath());
    process->setStandardErrorFile(QProcess::nullDevice());
    process->start();
    if (!process->waitForStarted(kStartTimeoutMs)) {
        qWarning() << "Andiya could not start Python plugin" << filter.id
                   << "-" << process->errorString();
        return false;
    }

    filter.process = std::move(process);
    return true;
}

bool PythonImageFilterHost::sendFrame(LoadedFilter &filter, const QImage &input,
                                      qint64 timestampUs, QImage &output) const
{
    if (filter.failed || !ensureStarted(filter)) {
        filter.failed = true;
        return false;
    }

    QProcess *process = filter.process.get();
    const quint64 dataSize = static_cast<quint64>(input.sizeInBytes());

    QByteArray header;
    header.reserve(kRequestHeaderSize);
    {
        QDataStream stream(&header, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << kRequestMagic
               << static_cast<quint32>(input.width())
               << static_cast<quint32>(input.height())
               << static_cast<quint32>(input.bytesPerLine())
               << static_cast<quint32>(1) // BGRA32 byte order, matching NativeImageFilterHost.
               << static_cast<qint64>(timestampUs)
               << dataSize;
    }

    if (process->write(header) != header.size() ||
        process->write(reinterpret_cast<const char *>(input.constBits()),
                       static_cast<qint64>(dataSize)) != static_cast<qint64>(dataSize)) {
        qWarning() << "Andiya could not send a frame to Python plugin" << filter.id;
        filter.failed = true;
        return false;
    }
    QElapsedTimer writeDeadline;writeDeadline.start();
    while (process->bytesToWrite()>0) {
      if (writeDeadline.elapsed()>=kWriteTimeoutMs || !process->waitForBytesWritten(kWriteTimeoutMs-int(writeDeadline.elapsed()))) {
        qWarning() << "Andiya timed out sending a frame to Python plugin" << filter.id;
        filter.failed = true;
        return false;
    }

    }
    QByteArray responseHeader;
    if (!readExact(process, kResponseHeaderSize, responseHeader)) {
        qWarning() << "Andiya timed out waiting for a response from Python plugin" << filter.id;
        filter.failed = true;
        return false;
    }

    quint32 magic = 0;
    qint32 status = 0;
    quint32 width = 0;
    quint32 height = 0;
    quint32 stride = 0;
    quint32 pixelFormat = 0;
    quint64 responseDataSize = 0;
    {
        QDataStream stream(responseHeader);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> magic >> status >> width >> height >> stride >> pixelFormat >> responseDataSize;
    }

    if (magic != kResponseMagic) {
        qWarning() << "Andiya received a malformed response from Python plugin" << filter.id;
        filter.failed = true;
        return false;
    }
    if (status != 0) {
        if (responseDataSize != 0 || status != 1) { filter.failed = true; process->kill(); }
        return false;
    }
    if (pixelFormat != 1 || width != static_cast<quint32>(input.width()) ||
        height != static_cast<quint32>(input.height()) ||
        stride != static_cast<quint32>(input.bytesPerLine()) ||
        responseDataSize != dataSize) {
        qWarning() << "Andiya bypassed a frame from Python plugin" << filter.id
                   << "because it returned an unexpected frame shape.";
        filter.failed = true; process->kill();
        return false;
    }

    QByteArray payload;
    payload.reserve(static_cast<int>(dataSize));
    if (!readExact(process, static_cast<qint64>(dataSize), payload)) {
        qWarning() << "Andiya timed out reading frame data from Python plugin" << filter.id;
        filter.failed = true;
        return false;
    }

    QImage result(input.size(), QImage::Format_ARGB32);
    if (result.isNull() || static_cast<quint64>(result.sizeInBytes()) != dataSize) {
        return false;
    }
    std::memcpy(result.bits(), payload.constData(), static_cast<size_t>(dataSize));
    output = std::move(result);
    return true;
}

QImage PythonImageFilterHost::apply(const QImage &source, qint64 timestampUs) const
{
    if (source.isNull() || !hasEnabledFilters()) {
        return source;
    }

    QImage current = source.convertToFormat(QImage::Format_ARGB32);
    for (LoadedFilter &filter : m_filters) {
        if (!filter.enabled || filter.failed) {
            continue;
        }
        QImage output;
        if (sendFrame(filter, current, timestampUs, output)) {
            current = std::move(output);
        }
        // On failure the frame is silently bypassed (current is unchanged);
        // sendFrame() already logged the reason.
    }
    return current;
}

QImage PythonImageFilterHost::applyOne(const QString &id, const QImage &source, qint64 timestampUs) const
{
    if (source.isNull() || source.width()>16384 || source.height()>16384 || source.sizeInBytes()>256*1024*1024) {
        return source;
    }
    for (LoadedFilter &filter : m_filters) {
        if (filter.id != id) {
            continue;
        }
        if (!filter.enabled || filter.failed) {
            return source;
        }
        const QImage current = source.convertToFormat(QImage::Format_ARGB32);
        QImage output;
        if (sendFrame(filter, current, timestampUs, output)) {
            return output;
        }
        if (filter.failed && filter.process) { filter.process->kill(); filter.process->waitForFinished(1000); filter.process.reset(); }
        return current;
    }
    return source;
}

bool PythonImageFilterHost::loadNative(const QString &id, const QString &path, QString *error) {
    if (!QFileInfo::exists(path) || !QFileInfo::exists(QCoreApplication::applicationDirPath() + QStringLiteral("/AndiyaFilterWorker")
#ifdef Q_OS_WIN
        + QStringLiteral(".exe")
#endif
)) {
        if (error) *error = "Native plugin or worker executable is missing.";
        return false;
    }
    LoadedFilter filter;
    filter.id = id; filter.scriptPath = path; filter.native = true;
    m_filters.push_back(std::move(filter));
    return true;
}
void PythonImageFilterHost::setParameters(const QString &id, const QString &json) {
    for (auto &filter : m_filters) if (filter.id == id && filter.parameters != json) {
        if (filter.process) { filter.process->kill(); filter.process->waitForFinished(1000); filter.process.reset(); }
        filter.parameters = json; filter.failed = false;
    }
}
