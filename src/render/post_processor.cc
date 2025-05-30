#include "render/post_processor.h"

namespace fallout {

bool PostProcessor::init(VkDevice device, VkFormat format, VkExtent2D extent)
{
    m_device = device;
    m_format = format;
    m_extent = extent;

    VkAttachmentDescription attachment{};
    attachment.format = format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &attachment;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(device, &rpInfo, nullptr, &postProcessPass) != VK_SUCCESS)
        return false;

    for (auto& eff : effects)
        eff->init(device, postProcessPass, extent);

    return true;
}

void PostProcessor::destroy(VkDevice device)
{
    for (auto& eff : effects)
        eff->destroy(device);
    effects.clear();
    if (postProcessPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, postProcessPass, nullptr);
        postProcessPass = VK_NULL_HANDLE;
    }
    m_device = VK_NULL_HANDLE;
}

void PostProcessor::addEffect(std::unique_ptr<PostEffect> effect)
{
    effects.push_back(std::move(effect));
}

void PostProcessor::apply(VkCommandBuffer cmd, VkImage src, VkImage dst)
{
    VkImageBlit blit{};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[1] = {static_cast<int32_t>(m_extent.width), static_cast<int32_t>(m_extent.height), 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[1] = {static_cast<int32_t>(m_extent.width), static_cast<int32_t>(m_extent.height), 1};

    VkImage in = src;
    VkImage out = dst;
    for (size_t i = 0; i < effects.size(); ++i) {
        effects[i]->apply(cmd, in, out, blit);
        std::swap(in, out);
    }

    if (effects.size() % 2 != 0) {
        // Result is in 'in', copy to dst
        blit.srcOffsets[1] = {static_cast<int32_t>(m_extent.width), static_cast<int32_t>(m_extent.height), 1};
        vkCmdBlitImage(cmd, in, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
    }
}

void BlitEffect::apply(VkCommandBuffer cmd, VkImage src, VkImage dst, const VkImageBlit& region)
{
    vkCmdBlitImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, m_filter);
}

} // namespace fallout

