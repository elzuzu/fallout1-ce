#ifndef FALLOUT_RENDER_POST_PROCESSOR_H_
#define FALLOUT_RENDER_POST_PROCESSOR_H_

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace fallout {

class PostEffect {
public:
    virtual ~PostEffect() = default;
    virtual void init(VkDevice device, VkRenderPass pass, VkExtent2D extent) {}
    virtual void destroy(VkDevice device) {}
    virtual void apply(VkCommandBuffer cmd, VkImage src, VkImage dst,
        const VkImageBlit& region) = 0;
};

class PostProcessor {
public:
    PostProcessor() = default;

    bool init(VkDevice device, VkFormat format, VkExtent2D extent);
    void destroy(VkDevice device);

    void addEffect(std::unique_ptr<PostEffect> effect);
    void apply(VkCommandBuffer cmd, VkImage src, VkImage dst);

private:
    std::vector<std::unique_ptr<PostEffect>> effects;
    VkRenderPass postProcessPass = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkExtent2D m_extent{};
    VkFormat m_format = VK_FORMAT_UNDEFINED;
};

// Simple blit effect used as placeholder for more complex filters.
class BlitEffect : public PostEffect {
public:
    explicit BlitEffect(VkFilter filter) : m_filter(filter) {}
    void apply(VkCommandBuffer cmd, VkImage src, VkImage dst,
        const VkImageBlit& region) override;

private:
    VkFilter m_filter = VK_FILTER_LINEAR;
};

// Placeholder classes for future implementations of FXAA, SMAA, CRT, etc.
class FxaaEffect : public BlitEffect {
public:
    FxaaEffect() : BlitEffect(VK_FILTER_LINEAR) {}
};

class SmaaEffect : public BlitEffect {
public:
    SmaaEffect() : BlitEffect(VK_FILTER_LINEAR) {}
};

class UpscaleEffect : public BlitEffect {
public:
    explicit UpscaleEffect(bool nearest)
        : BlitEffect(nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR) {}
};

class CrtEffect : public BlitEffect {
public:
    CrtEffect() : BlitEffect(VK_FILTER_LINEAR) {}
};

class ScanlineEffect : public BlitEffect {
public:
    ScanlineEffect() : BlitEffect(VK_FILTER_LINEAR) {}
};

} // namespace fallout

#endif /* FALLOUT_RENDER_POST_PROCESSOR_H_ */
