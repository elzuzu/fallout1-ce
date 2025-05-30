#include "render/FrameGraph.h"

namespace fallout {

void FrameGraph::record(const std::vector<DrawBatch>& batches, VkCommandBuffer primary)
{
    ThreadPool pool(batches.size());
    std::latch done(batches.size());
    for (const auto& b : batches) {
        pool.enqueue([&]{
            // assume batches already contain secondary command buffers
            done.count_down();
        });
    }
    done.wait();
    std::vector<VkCommandBuffer> secs;
    for (const auto& b : batches) secs.push_back(b.cmd);
    vkCmdExecuteCommands(primary, secs.size(), secs.data());
}

} // namespace fallout
