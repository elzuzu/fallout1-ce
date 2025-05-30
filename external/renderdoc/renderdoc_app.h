#ifndef RENDERDOC_APP_H
#define RENDERDOC_APP_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define eRENDERDOC_API_Version_1_6_0 10600
typedef struct RENDERDOC_API_1_6_0 {
    void (*StartFrameCapture)(void* device, void* window);
    bool (*EndFrameCapture)(void* device, void* window);
    uint32_t (*GetOverlayBits)(void);
    void (*MaskOverlayBits)(uint32_t andMask, uint32_t eorMask);
    void (*LaunchReplayUI)(uint32_t connectTarget, const char* cmdline);
} RENDERDOC_API_1_6_0;

typedef int (*pRENDERDOC_GetAPI)(uint32_t version, void** outAPIPointers);
#ifdef __cplusplus
}
#endif
#endif // RENDERDOC_APP_H
