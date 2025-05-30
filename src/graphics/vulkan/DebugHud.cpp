#include "graphics/vulkan/DebugHud.h"
#include <SDL.h>
#include <dlfcn.h>
#include <imgui.h>

namespace fallout {

DebugHud gDebugHud;

DebugHud& DebugHud::instance() { return gDebugHud; }

bool DebugHud::init(VkInstance instance, VkDevice device)
{
    (void)instance; (void)device;
    void* lib = SDL_LoadObject("librenderdoc.so");
    if (lib) {
        pRENDERDOC_GetAPI getApi = (pRENDERDOC_GetAPI)SDL_LoadFunction(lib, "RENDERDOC_GetAPI");
        if (getApi) {
            getApi(eRENDERDOC_API_Version_1_6_0, (void**)&m_rdoc);
        }
    }
    return true;
}

void DebugHud::shutdown()
{
    m_rdoc = nullptr;
}

void DebugHud::begin(VkCommandBuffer)
{
    // overlay is drawn by ImGui later
}

void DebugHud::end(VkCommandBuffer)
{
    ImGui::Begin("GPU HUD", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("GPU frame %.2f ms", gVulkanDebugger.last_frame_ms());
    ImGui::End();
    if (m_requested && m_rdoc) {
        m_rdoc->LaunchReplayUI(1, nullptr);
        m_requested = false;
    }
}

void DebugHud::handleEvent(const SDL_Event& e)
{
    if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_F12) {
        m_requested = true;
    }
}

} // namespace fallout
