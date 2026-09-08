// MIT-licensed native inversion example. No Qt dependency.
#include <andiya/plugin_api.h>
#include <cstring>

namespace {
const char *ANDIYA_PLUGIN_CALL pluginId(void *) {
    return "example.native-invert";
}

int32_t ANDIYA_PLUGIN_CALL process(void *, const AndiyaFrame *input, AndiyaFrame *output) {
    if (!input || !output || input->struct_size < sizeof(AndiyaFrame) ||
        output->struct_size < sizeof(AndiyaFrame) || !input->data || !output->data ||
        input->pixel_format != ANDIYA_PIXEL_FORMAT_BGRA32 ||
        output->pixel_format != input->pixel_format ||
        !input->width || !input->height ||
        input->width != output->width || input->height != output->height ||
        input->stride != output->stride ||
        static_cast<uint64_t>(input->stride) < static_cast<uint64_t>(input->width) * 4) {
        return -1;
    }
    const uint64_t bytes = static_cast<uint64_t>(input->stride) * input->height;
    if (input->data_size != bytes || output->data_size != bytes ||
        bytes > 256ull * 1024 * 1024) {
        return -1;
    }
    const auto *source = static_cast<const unsigned char *>(input->data);
    auto *destination = static_cast<unsigned char *>(output->data);
    std::memcpy(destination, source, static_cast<size_t>(bytes));
    for (uint32_t y = 0; y < input->height; ++y) {
        for (uint32_t x = 0; x < input->width; ++x) {
            const uint64_t offset = static_cast<uint64_t>(y) * input->stride +
                                    static_cast<uint64_t>(x) * 4;
            for (unsigned channel = 0; channel < 3; ++channel) {
                destination[offset + channel] = 255 - source[offset + channel];
            }
        }
    }
    return 0;
}

void ANDIYA_PLUGIN_CALL destroy(void *) {}
}

extern "C" ANDIYA_PLUGIN_EXPORT int32_t ANDIYA_PLUGIN_CALL
andiya_plugin_initialize(uint32_t version, const AndiyaHostApi *host, AndiyaImageFilterApi *plugin) {
    if (version != ANDIYA_PLUGIN_API_VERSION || !host || !plugin ||
        host->struct_size < sizeof(AndiyaHostApi) ||
        host->api_version != ANDIYA_PLUGIN_API_VERSION ||
        plugin->struct_size < sizeof(AndiyaImageFilterApi)) {
        return -1;
    }
    plugin->struct_size = sizeof(AndiyaImageFilterApi);
    plugin->api_version = ANDIYA_PLUGIN_API_VERSION;
    plugin->plugin_context = nullptr;
    plugin->plugin_id = pluginId;
    plugin->process_frame = process;
    plugin->destroy = destroy;
    return 0;
}
