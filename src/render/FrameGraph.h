#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "render/ThreadPool.h"

namespace fallout {
struct DrawBatch { VkCommandBuffer cmd; };

class FrameGraph {
public:
    void record(const std::vector<DrawBatch>& batches, VkCommandBuffer primary);
};
}
