#pragma once

#include <QImage>
#include <QString>

#include <memory>
#include <vector>

class QProcess;

// Runs Python image-filter plugins out-of-process. Each enabled plugin gets
// its own long-lived interpreter process (lazily started on first use),
// reused for every subsequent frame, communicating over stdin/stdout with a
// tiny fixed binary header -- see runtime/python/andiya_python_host.py for
// the exact wire format. A crashing, hanging, or misbehaving Python plugin
// can only take down its own child process; it never touches Andiya's
// memory space and a failed plugin is simply bypassed from then on.
class PythonImageFilterHost final
{
public:
    PythonImageFilterHost();
    ~PythonImageFilterHost();

    PythonImageFilterHost(const PythonImageFilterHost &) = delete;
    PythonImageFilterHost &operator=(const PythonImageFilterHost &) = delete;

    // `scriptPath` is the user's plugin .py file; validity is checked here,
    // but the interpreter process itself is not started until first use.
    [[nodiscard]] bool load(const QString &manifestId,
                            const QString &scriptPath,
                            QString *errorMessage = nullptr);
    void setEnabled(const QString &id, bool enabled);
    void clear();
    bool loadNative(const QString &id, const QString &path, QString *error = nullptr);
    void setParameters(const QString &id, const QString &json);

        [[nodiscard]] bool isLoaded(const QString &id) const;
        [[nodiscard]] bool isEnabled(const QString &id) const;
        [[nodiscard]] bool hasEnabledFilters() const;
        [[nodiscard]] QImage apply(const QImage &source, qint64 timestampUs = 0) const;
        // Runs only the named filter, letting the caller (PluginListModel)
        // drive the overall pipeline order across native and Python filters.
        [[nodiscard]] QImage applyOne(const QString &id, const QImage &source, qint64 timestampUs = 0) const;

    // Locates a usable Python 3 interpreter on PATH, or an empty string if
    // none was found.
    [[nodiscard]] static QString findInterpreter();

private:
    struct LoadedFilter {
        QString id;
        QString scriptPath;
        std::unique_ptr<QProcess> process;
        bool enabled = false;
        bool failed = false;
        bool native = false;
        QString parameters = QStringLiteral("{}");
    };

    [[nodiscard]] bool sendFrame(LoadedFilter &filter, const QImage &input,
                                 qint64 timestampUs, QImage &output) const;
    [[nodiscard]] bool ensureStarted(LoadedFilter &filter) const;

    QString m_interpreter;
    // apply() talks to child processes (writing/reading pipes, lazily
    // starting them), which are logically an implementation detail rather
    // than part of the filter list itself, so the list is mutable to allow
    // this from a const apply().
    mutable std::vector<LoadedFilter> m_filters;
};
