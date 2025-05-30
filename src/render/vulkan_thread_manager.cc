#include "render/vulkan_thread_manager.h"

namespace fallout {

VulkanThreadManager gVulkanThread;

VulkanThreadManager::~VulkanThreadManager()
{
    stop();
}

void VulkanThreadManager::start()
{
    running = true;
    renderThread = std::thread(&VulkanThreadManager::threadLoop, this);
}

void VulkanThreadManager::stop()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        running = false;
    }
    queueCond.notify_all();
    if (renderThread.joinable())
        renderThread.join();
}

void VulkanThreadManager::submit(const RenderCommand& cmd)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        commandQueue.push(cmd);
    }
    queueCond.notify_one();
}

void VulkanThreadManager::threadLoop()
{
    while (true) {
        RenderCommand cmd;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCond.wait(lock, [this] { return !commandQueue.empty() || !running; });
            if (!running && commandQueue.empty())
                break;
            cmd = commandQueue.front();
            commandQueue.pop();
        }
        if (cmd.func)
            cmd.func();
    }
}

} // namespace fallout
