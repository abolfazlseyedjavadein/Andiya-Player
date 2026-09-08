#ifndef ANDIYA_PLUGIN_API_H
#define ANDIYA_PLUGIN_API_H

#include <stdint.h>

#if defined(_WIN32)
#  define ANDIYA_PLUGIN_EXPORT __declspec(dllexport)
#  define ANDIYA_PLUGIN_CALL __cdecl
#else
#  define ANDIYA_PLUGIN_EXPORT __attribute__((visibility("default")))
#  define ANDIYA_PLUGIN_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ANDIYA_PLUGIN_API_VERSION 1u

typedef enum AndiyaPixelFormat {
    ANDIYA_PIXEL_FORMAT_BGRA32 = 1,
    ANDIYA_PIXEL_FORMAT_RGBA32 = 2,
    ANDIYA_PIXEL_FORMAT_RGBA64 = 3,
    ANDIYA_PIXEL_FORMAT_NV12 = 4
} AndiyaPixelFormat;

typedef enum AndiyaColorSpace {
    ANDIYA_COLOR_SPACE_SRGB = 1,
    ANDIYA_COLOR_SPACE_REC709 = 2,
    ANDIYA_COLOR_SPACE_DISPLAY_P3 = 3,
    ANDIYA_COLOR_SPACE_REC2020_PQ = 4
} AndiyaColorSpace;

typedef struct AndiyaFrame {
    uint32_t struct_size;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t pixel_format;
    uint32_t color_space;
    int64_t timestamp_us;
    void *data;
    uint64_t data_size;
} AndiyaFrame;

typedef struct AndiyaHostApi {
    uint32_t struct_size;
    uint32_t api_version;
    void *host_context;
    void *(ANDIYA_PLUGIN_CALL *allocate)(void *host_context, uint64_t size);
    void (ANDIYA_PLUGIN_CALL *release)(void *host_context, void *memory);
    void (ANDIYA_PLUGIN_CALL *log)(void *host_context, int32_t level, const char *message_utf8);
} AndiyaHostApi;

typedef struct AndiyaImageFilterApi {
    uint32_t struct_size;
    uint32_t api_version;
    void *plugin_context;
    const char *(ANDIYA_PLUGIN_CALL *plugin_id)(void *plugin_context);
    int32_t (ANDIYA_PLUGIN_CALL *process_frame)(
        void *plugin_context,
        const AndiyaFrame *input,
        AndiyaFrame *output);
    void (ANDIYA_PLUGIN_CALL *destroy)(void *plugin_context);
} AndiyaImageFilterApi;

ANDIYA_PLUGIN_EXPORT int32_t ANDIYA_PLUGIN_CALL andiya_plugin_initialize(
    uint32_t host_api_version,
    const AndiyaHostApi *host,
    AndiyaImageFilterApi *plugin);

#ifdef __cplusplus
}
#endif

#endif
