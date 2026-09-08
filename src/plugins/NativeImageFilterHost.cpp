#include "NativeImageFilterHost.h"

#include <QDebug>
#include <QLibrary>

#include <cstdlib>
#include <utility>

namespace {
using InitializeFunction = int32_t (ANDIYA_PLUGIN_CALL *)(
    uint32_t, const AndiyaHostApi *, AndiyaImageFilterApi *);

AndiyaFrame makeFrame(QImage &image, qint64 timestampUs)
{
    AndiyaFrame frame{};
    frame.struct_size = sizeof(AndiyaFrame);
    frame.width = static_cast<uint32_t>(image.width());
    frame.height = static_cast<uint32_t>(image.height());
    frame.stride = static_cast<uint32_t>(image.bytesPerLine());
    frame.pixel_format = ANDIYA_PIXEL_FORMAT_BGRA32;
    frame.color_space = ANDIYA_COLOR_SPACE_SRGB;
    frame.timestamp_us = timestampUs;
    frame.data = image.bits();
    frame.data_size = static_cast<uint64_t>(image.sizeInBytes());
    return frame;
}
}

NativeImageFilterHost::NativeImageFilterHost()
{
    m_hostApi.struct_size = sizeof(AndiyaHostApi);
    m_hostApi.api_version = ANDIYA_PLUGIN_API_VERSION;
    m_hostApi.host_context = this;
    m_hostApi.allocate = &NativeImageFilterHost::allocate;
    m_hostApi.release = &NativeImageFilterHost::release;
    m_hostApi.log = &NativeImageFilterHost::log;
}

NativeImageFilterHost::~NativeImageFilterHost()
{
    clear();
}

bool NativeImageFilterHost::load(const QString &manifestId,
                                 const QString &libraryPath,
                                 QString *errorMessage)
{
    if (isLoaded(manifestId)) {
        return true;
    }

    auto library = std::make_unique<QLibrary>(libraryPath);
    library->setLoadHints(QLibrary::ResolveAllSymbolsHint);
    if (!library->load()) {
        if (errorMessage) {
            *errorMessage = library->errorString();
        }
        return false;
    }

    const auto initialize = reinterpret_cast<InitializeFunction>(
        library->resolve("andiya_plugin_initialize"));
    if (!initialize) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Missing andiya_plugin_initialize export: %1")
                                .arg(library->errorString());
        }
        library->unload();
        return false;
    }

    AndiyaImageFilterApi api{};
    api.struct_size = sizeof(AndiyaImageFilterApi);
    const int32_t result = initialize(ANDIYA_PLUGIN_API_VERSION, &m_hostApi, &api);
    if (result != 0 || api.struct_size < sizeof(AndiyaImageFilterApi) ||
        api.api_version != ANDIYA_PLUGIN_API_VERSION ||
        !api.plugin_id || !api.process_frame || !api.destroy) {
        if (api.destroy && api.plugin_context) {
            api.destroy(api.plugin_context);
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("Plugin initialization or ABI validation failed (%1).")
                                .arg(result);
        }
        library->unload();
        return false;
    }

    const char *reportedId = api.plugin_id(api.plugin_context);
    if (!reportedId || QString::fromUtf8(reportedId) != manifestId) {
        api.destroy(api.plugin_context);
        if (errorMessage) {
            *errorMessage = QStringLiteral("The library ID does not match its manifest.");
        }
        library->unload();
        return false;
    }

    LoadedFilter filter;
    filter.id = manifestId;
    filter.path = libraryPath;
    filter.library = std::move(library);
    filter.api = api;
    m_filters.push_back(std::move(filter));
    return true;
}

void NativeImageFilterHost::setEnabled(const QString &id, bool enabled)
{
    for (LoadedFilter &filter : m_filters) {
        if (filter.id == id) {
            filter.enabled = enabled;
            return;
        }
    }
}

void NativeImageFilterHost::clear()
{
    for (auto iterator = m_filters.rbegin(); iterator != m_filters.rend(); ++iterator) {
        LoadedFilter &filter = *iterator;
        if (filter.api.destroy && filter.api.plugin_context) {
            filter.api.destroy(filter.api.plugin_context);
            filter.api.plugin_context = nullptr;
        }
        if (filter.library) {
            filter.library->unload();
        }
    }
    m_filters.clear();
}

bool NativeImageFilterHost::isLoaded(const QString &id) const
{
    for (const LoadedFilter &filter : m_filters) {
        if (filter.id == id) {
            return true;
        }
    }
    return false;
}

bool NativeImageFilterHost::isEnabled(const QString &id) const
{
    for (const LoadedFilter &filter : m_filters) {
        if (filter.id == id) {
            return filter.enabled;
        }
    }
    return false;
}

bool NativeImageFilterHost::hasEnabledFilters() const
{
    for (const LoadedFilter &filter : m_filters) {
        if (filter.enabled) {
            return true;
        }
    }
    return false;
}

QImage NativeImageFilterHost::runFilter(const LoadedFilter &filter, const QImage &source, qint64 timestampUs) const
{
    QImage current = source.convertToFormat(QImage::Format_ARGB32);
    QImage output(current.size(), QImage::Format_ARGB32);
    if (output.isNull()) {
        qWarning() << "Andiya could not allocate a filter output frame for" << filter.id;
        return current;
    }

    AndiyaFrame inputFrame = makeFrame(current, timestampUs);
    AndiyaFrame outputFrame = makeFrame(output, timestampUs);
    const void *expectedOutputData = outputFrame.data;
    const int32_t result = filter.api.process_frame(
        filter.api.plugin_context, &inputFrame, &outputFrame);

    const bool outputValid = result == 0 && outputFrame.data == expectedOutputData &&
                             outputFrame.width == inputFrame.width &&
                             outputFrame.height == inputFrame.height &&
                             outputFrame.stride == static_cast<uint32_t>(output.bytesPerLine()) &&
                             outputFrame.pixel_format == ANDIYA_PIXEL_FORMAT_BGRA32 &&
                             outputFrame.data_size == static_cast<uint64_t>(output.sizeInBytes());
    if (!outputValid) {
        qWarning() << "Andiya bypassed filter" << filter.id
                   << "because processing failed or returned an invalid frame:" << result;
        return current;
    }
    return output;
}

QImage NativeImageFilterHost::apply(const QImage &source, qint64 timestampUs) const
{
    if (source.isNull() || !hasEnabledFilters()) {
        return source;
    }

    QImage current = source.convertToFormat(QImage::Format_ARGB32);
    for (const LoadedFilter &filter : m_filters) {
        if (!filter.enabled) {
            continue;
        }
        current = runFilter(filter, current, timestampUs);
    }
    return current;
}

QImage NativeImageFilterHost::applyOne(const QString &id, const QImage &source, qint64 timestampUs) const
{
    if (source.isNull()) {
        return source;
    }
    for (const LoadedFilter &filter : m_filters) {
        if (filter.id == id) {
            if (!filter.enabled) {
                return source;
            }
            return runFilter(filter, source, timestampUs);
        }
    }
    return source;
}

void *ANDIYA_PLUGIN_CALL NativeImageFilterHost::allocate(void *, uint64_t size)
{
    return size == 0 ? nullptr : std::malloc(static_cast<size_t>(size));
}

void ANDIYA_PLUGIN_CALL NativeImageFilterHost::release(void *, void *memory)
{
    std::free(memory);
}

void ANDIYA_PLUGIN_CALL NativeImageFilterHost::log(void *, int32_t level,
                                                   const char *messageUtf8)
{
    const QString message = QString::fromUtf8(messageUtf8 ? messageUtf8 : "");
    if (level >= 3) {
        qCritical().noquote() << "Andiya plugin:" << message;
    } else if (level == 2) {
        qWarning().noquote() << "Andiya plugin:" << message;
    } else {
        qInfo().noquote() << "Andiya plugin:" << message;
    }
}

bool NativeImageFilterHost::configure(const QString &id, const QByteArray &json) {
    using Configure = int32_t (ANDIYA_PLUGIN_CALL *)(void *, const char *);
    for (auto &filter : m_filters) if (filter.id == id) {
        const auto callback = reinterpret_cast<Configure>(filter.library->resolve("andiya_plugin_configure"));
        return !callback || callback(filter.api.plugin_context, json.constData()) == 0;
    }
    return false;
}
