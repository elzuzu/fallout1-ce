#include "graphics/fallout_sprite_renderer.hpp"
#include "graphics/vulkan/vk_swapchain_production.hpp"
#include <algorithm>
#include <cstring>
#include <chrono>

bool FalloutSpriteRenderer::initialize(VkDevice dev, VmaAllocator alloc, VkRenderPass pass, uint32_t frameCount) {
    device = dev;
    allocator = alloc;
    renderPass = pass;

    frameData.resize(frameCount);
    descriptorSets.resize(frameCount);

    if (!createQuadGeometry()) {
        logError("Failed to create quad geometry");
        return false;
    }

    if (!createSamplers()) {
        logError("Failed to create samplers");
        return false;
    }

    if (!createDescriptorSetLayout()) {
        logError("Failed to create descriptor set layout");
        return false;
    }

    if (!createPipelineLayout()) {
        logError("Failed to create pipeline layout");
        return false;
    }

    if (!createGraphicsPipeline()) {
        logError("Failed to create graphics pipeline");
        return false;
    }

    if (!createBuffers()) {
        logError("Failed to create buffers");
        return false;
    }

    if (!createDescriptorPool()) {
        logError("Failed to create descriptor pool");
        return false;
    }

    if (!allocateDescriptorSets()) {
        logError("Failed to allocate descriptor sets");
        return false;
    }

    spriteBatch.reserve(MAX_SPRITES_PER_BATCH);
    textureViews.fill(VK_NULL_HANDLE);
    logInfo("Fallout sprite renderer initialized (max %u sprites per batch)", MAX_SPRITES_PER_BATCH);
    return true;
}

void FalloutSpriteRenderer::beginFrame(uint32_t frameIndex) {
    currentFrame = frameIndex;
    spriteBatch.clear();
}

void FalloutSpriteRenderer::setCamera(const glm::mat4& view, const glm::mat4& proj, const glm::vec2& screenSize, const glm::vec2& cameraPos) {
    FalloutCameraUBO cameraData{};
    cameraData.viewMatrix = view;
    cameraData.projMatrix = proj;
    cameraData.screenSize = screenSize;
    cameraData.cameraPos = cameraPos;
    using namespace std::chrono;
    cameraData.time = duration_cast<duration<float>>(steady_clock::now().time_since_epoch()).count();
    std::memcpy(frameData[currentFrame].cameraMapped, &cameraData, sizeof(FalloutCameraUBO));
}

void FalloutSpriteRenderer::setRenderSettings(const FalloutRenderSettings& settings) {
    std::memcpy(frameData[currentFrame].settingsMapped, &settings, sizeof(FalloutRenderSettings));
}

void FalloutSpriteRenderer::drawSprite(const glm::vec2& position, const glm::vec2& size, const glm::vec4& uvRect,
                   uint32_t textureIndex, const glm::vec4& color, float rotation, float depth) {
    if (spriteBatch.size() >= MAX_SPRITES_PER_BATCH) {
        logWarning("Sprite batch full, flushing early");
        flush();
    }
    SpriteInstance instance{};
    instance.position = position;
    instance.size = size;
    instance.uvRect = uvRect;
    instance.color = color;
    instance.rotation = rotation;
    instance.depth = depth;
    instance.textureIndex = textureIndex;
    spriteBatch.push_back(instance);
}

void FalloutSpriteRenderer::drawFalloutSprite(const FalloutSprite& sprite, uint32_t frameIndex, uint32_t direction,
                          const glm::vec2& position, const glm::vec2& scale, const glm::vec4& color, float depth) {
    if (frameIndex >= sprite.frameCount) {
        frameIndex = 0;
    }
    uint32_t actualFrame = direction * sprite.framesPerDirection + frameIndex;
    if (actualFrame >= sprite.frameUVs.size()) {
        actualFrame = 0;
    }
    glm::vec4 uvRect = sprite.frameUVs[actualFrame];
    const FalloutFrame& frame = sprite.frames[actualFrame];
    glm::vec2 size = glm::vec2(frame.width, frame.height) * scale;
    glm::vec2 offset = glm::vec2(frame.offsetX, frame.offsetY) * scale;
    drawSprite(position + offset, size, uvRect, 0, color, 0.0f, depth);
}

void FalloutSpriteRenderer::setTexture(uint32_t index, VkImageView textureView) {
    if (index < MAX_TEXTURES) {
        textureViews[index] = textureView;
        updateDescriptorSets();
    }
}

void FalloutSpriteRenderer::flush() {
    if (spriteBatch.empty()) {
        return;
    }
    std::sort(spriteBatch.begin(), spriteBatch.end(),
               [](const SpriteInstance& a, const SpriteInstance& b) { return a.depth < b.depth; });
    std::memcpy(frameData[currentFrame].instanceMapped, spriteBatch.data(), spriteBatch.size() * sizeof(SpriteInstance));
    logInfo("Flushed %zu sprites", spriteBatch.size());
    spriteBatch.clear();
}

void FalloutSpriteRenderer::recordDrawCommands(VkCommandBuffer commandBuffer) {
    if (spriteBatch.empty()) {
        return;
    }
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
    VkBuffer vertexBuffers[] = {quadVertexBuffer, frameData[currentFrame].instanceBuffer};
    VkDeviceSize offsets[] = {0, 0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, quadIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, 6, static_cast<uint32_t>(spriteBatch.size()), 0, 0, 0);
}

void FalloutSpriteRenderer::setPixelPerfectMode(bool enabled) {
    pixelPerfectMode = enabled;
    updateDescriptorSets();
}

// Placeholder implementations for the private helper methods
bool FalloutSpriteRenderer::createQuadGeometry() { return true; }
bool FalloutSpriteRenderer::createSamplers() { return true; }
bool FalloutSpriteRenderer::createDescriptorSetLayout() { return true; }
bool FalloutSpriteRenderer::createPipelineLayout() { return true; }
bool FalloutSpriteRenderer::createGraphicsPipeline() { return true; }
bool FalloutSpriteRenderer::createBuffers() { return true; }
bool FalloutSpriteRenderer::createDescriptorPool() { return true; }
bool FalloutSpriteRenderer::allocateDescriptorSets() { return true; }
void FalloutSpriteRenderer::updateDescriptorSets() {}
void FalloutSpriteRenderer::uploadBufferData(const void* , size_t , VkBuffer ) {}

