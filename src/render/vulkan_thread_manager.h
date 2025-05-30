#ifndef FALLOUT_RENDER_VULKAN_THREAD_MANAGER_H_
#define FALLOUT_RENDER_VULKAN_THREAD_MANAGER_H_

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace fallout {

struct RenderCommand {
    std::function<void()> func;
};

class VulkanThreadManager {
public:
    VulkanThreadManager() = default;
    ~VulkanThreadManager();

    void start();
    void stop();
    void submit(const RenderCommand& cmd);

private:
    void threadLoop();

    std::thread renderThread;
    std::queue<RenderCommand> commandQueue;
    std::mutex queueMutex;
    std::condition_variable queueCond;
    bool running = false;
};

extern VulkanThreadManager gVulkanThread;

} // namespace fallout

#endif /* FALLOUT_RENDER_VULKAN_THREAD_MANAGER_H_ */
