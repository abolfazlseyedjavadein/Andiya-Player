#include "andiya/plugin_api.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <new>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
struct PluginContext {
    const AndiyaHostApi *host = nullptr;
    double contrast=1.08;
    double warmth=1.0;
};

uint8_t clampChannel(int value)
{
    return static_cast<uint8_t>(std::clamp(value, 0, 255));
}

const char *ANDIYA_PLUGIN_CALL pluginId(void *)
{
    return "example.soft-contrast";
}

int32_t ANDIYA_PLUGIN_CALL processFrame(void *context, const AndiyaFrame *input,
                                        AndiyaFrame *output)
{
    if (!input || !output || !input->data || !output->data ||
        input->pixel_format != ANDIYA_PIXEL_FORMAT_BGRA32 ||
        output->pixel_format != ANDIYA_PIXEL_FORMAT_BGRA32 ||
        input->width != output->width || input->height != output->height ||
        input->stride < static_cast<uint64_t>(input->width) * 4 ||
        output->stride < static_cast<uint64_t>(output->width) * 4 ||
        input->data_size < static_cast<uint64_t>(input->stride) * input->height ||
        output->data_size < static_cast<uint64_t>(output->stride) * output->height ||
        input->data_size > output->data_size) {
        return -1;
    }

    std::memcpy(output->data, input->data, static_cast<size_t>(input->data_size));
    const auto *source = static_cast<const uint8_t *>(input->data);
    auto *destination = static_cast<uint8_t *>(output->data);

    for (uint32_t y = 0; y < input->height; ++y) {
        const uint8_t *sourceRow = source + static_cast<size_t>(y) * input->stride;
        uint8_t *destinationRow = destination + static_cast<size_t>(y) * output->stride;
        for (uint32_t x = 0; x < input->width; ++x) {
            const uint8_t *sourcePixel = sourceRow + static_cast<size_t>(x) * 4;
            uint8_t *destinationPixel = destinationRow + static_cast<size_t>(x) * 4;

            const auto *settings=static_cast<PluginContext *>(context);
            const auto contrast = [settings](int channel) {return int((channel-128)*settings->contrast+128);};
            destinationPixel[0] = clampChannel(contrast(sourcePixel[0]) - int(3*settings->warmth)); // Blue
            destinationPixel[1] = clampChannel(contrast(sourcePixel[1]) + int(settings->warmth)); // Green
            destinationPixel[2] = clampChannel(contrast(sourcePixel[2]) + int(6*settings->warmth)); // Red
            destinationPixel[3] = sourcePixel[3];
        }
    }
    return 0;
}

void ANDIYA_PLUGIN_CALL destroy(void *context)
{
    delete static_cast<PluginContext *>(context);
}
}

extern "C" ANDIYA_PLUGIN_EXPORT int32_t ANDIYA_PLUGIN_CALL andiya_plugin_initialize(
    uint32_t hostApiVersion, const AndiyaHostApi *host, AndiyaImageFilterApi *plugin)
{
    if (hostApiVersion != ANDIYA_PLUGIN_API_VERSION || !host || !plugin ||
        host->struct_size < sizeof(AndiyaHostApi) ||
        plugin->struct_size < sizeof(AndiyaImageFilterApi)) {
        return -1;
    }

    auto *context = new (std::nothrow) PluginContext{host};
    if (!context) {
        return -2;
    }

    plugin->struct_size = sizeof(AndiyaImageFilterApi);
    plugin->api_version = ANDIYA_PLUGIN_API_VERSION;
    plugin->plugin_context = context;
    plugin->plugin_id = &pluginId;
    plugin->process_frame = &processFrame;
    plugin->destroy = &destroy;

    if (host->log) {
        host->log(host->host_context, 1, "Soft Contrast initialized.");
    }
    return 0;
}

extern "C" ANDIYA_PLUGIN_EXPORT int32_t ANDIYA_PLUGIN_CALL andiya_plugin_configure(void *context,const char *json) {
    if(!context||!json)return -1;
    auto *settings=static_cast<PluginContext *>(context);
    const auto values=QJsonDocument::fromJson(json).object();
    settings->contrast=std::clamp(values.value("contrast").toDouble(1.08),0.5,2.0);
    settings->warmth=std::clamp(values.value("warmth").toDouble(1.0),0.0,3.0);
    return 0;
}
