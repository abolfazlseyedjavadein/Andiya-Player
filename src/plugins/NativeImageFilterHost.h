#pragma once

#include <QImage>
#include <QString>

#include <memory>
#include <vector>

#include "andiya/plugin_api.h"

class QLibrary;

class NativeImageFilterHost final
{
public:
    NativeImageFilterHost();
    ~NativeImageFilterHost();

    NativeImageFilterHost(const NativeImageFilterHost &) = delete;
    NativeImageFilterHost &operator=(const NativeImageFilterHost &) = delete;

    [[nodiscard]] bool load(const QString &manifestId,
                            const QString &libraryPath,
                            QString *errorMessage = nullptr);
    void setEnabled(const QString &id, bool enabled);
    void clear();
    bool configure(const QString &id, const QByteArray &json);

    [[nodiscard]] bool isLoaded(const QString &id) const;
    [[nodiscard]] bool isEnabled(const QString &id) const;
    [[nodiscard]] bool hasEnabledFilters() const;
    // Runs every enabled filter, in load order. Kept for callers that do not
    // care about interleaving native and out-of-process filters.
    [[nodiscard]] QImage apply(const QImage &source, qint64 timestampUs = 0) const;
    // Runs only the named filter (if loaded and enabled), letting the caller
    // (PluginListModel) drive the overall pipeline order across both native
    // and Python filters, e.g. for the reorderable "filter graph" UI.
    [[nodiscard]] QImage applyOne(const QString &id, const QImage &source, qint64 timestampUs = 0) const;

private:
    struct LoadedFilter {
        QString id;
        QString path;
        std::unique_ptr<QLibrary> library;
        AndiyaImageFilterApi api{};
        bool enabled = false;
    };

    [[nodiscard]] QImage runFilter(const LoadedFilter &filter, const QImage &source, qint64 timestampUs) const;

    static void *ANDIYA_PLUGIN_CALL allocate(void *hostContext, uint64_t size);
    static void ANDIYA_PLUGIN_CALL release(void *hostContext, void *memory);
    static void ANDIYA_PLUGIN_CALL log(void *hostContext, int32_t level,
                                      const char *messageUtf8);

    AndiyaHostApi m_hostApi{};
    std::vector<LoadedFilter> m_filters;
};
