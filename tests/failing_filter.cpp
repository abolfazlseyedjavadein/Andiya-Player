#include "andiya/plugin_api.h"

namespace {
const char *ANDIYA_PLUGIN_CALL id(void *) { return "test.native-failure"; }
int32_t ANDIYA_PLUGIN_CALL process(void *, const AndiyaFrame *input, AndiyaFrame *output) {
    if (input->timestamp_us == 0) return -1;
    ++output->width;
    return 0;
}
void ANDIYA_PLUGIN_CALL destroy(void *) {}
}

extern "C" ANDIYA_PLUGIN_EXPORT int32_t ANDIYA_PLUGIN_CALL andiya_plugin_initialize(
    uint32_t, const AndiyaHostApi *, AndiyaImageFilterApi *api) {
    api->api_version = ANDIYA_PLUGIN_API_VERSION;
    api->plugin_id = id;
    api->process_frame = process;
    api->destroy = destroy;
    return 0;
}
