#pragma once
#include <vulkan/vulkan.h>
#include <SDL_events.h>
#include "render/vulkan_debugger.h"
#include "external/renderdoc/renderdoc_app.h"

namespace fallout {
class DebugHud {
public:
    static DebugHud& instance();
    bool init(VkInstance instance, VkDevice device);
    void shutdown();
    void begin(VkCommandBuffer cmd);
    void end(VkCommandBuffer cmd);
    void handleEvent(const SDL_Event& e);
private:
    DebugHud() = default;
    RENDERDOC_API_1_6_0* m_rdoc = nullptr;
    bool m_requested = false;
};

extern DebugHud gDebugHud;
}
