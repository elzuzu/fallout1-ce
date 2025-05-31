#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

struct SpriteInstance {
    glm::vec2 position;       // World position
    glm::vec2 size;           // Sprite size in world units
    glm::vec4 uvRect;         // UV coordinates (x, y, width, height)
    glm::vec4 color;          // Color tint (RGBA)
    float rotation;           // Rotation in radians
    float depth;              // Z-depth for sorting
    uint32_t textureIndex;    // Index into texture array
    uint32_t padding;
};

struct FalloutCameraUBO {
    alignas(16) glm::mat4 viewMatrix;
    alignas(16) glm::mat4 projMatrix;
    alignas(16) glm::vec2 screenSize;
    alignas(16) glm::vec2 cameraPos;
    float time;
    float padding[3];
};

struct FalloutRenderSettings {
    alignas(16) glm::vec3 ambientColor;
    float ambientIntensity;
    alignas(16) glm::vec3 globalTint;
    float gamma;
    float contrast;
    float brightness;
    float saturation;
    float pixelPerfect; // 0.0 = smooth, 1.0 = pixel perfect
};

class FalloutSpriteRenderer {
private:
    static constexpr uint32_t MAX_SPRITES_PER_BATCH = 10000;
    static constexpr uint32_t MAX_TEXTURES = 32;

    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;

    // Pipeline objects
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline spritePipeline = VK_NULL_HANDLE;

    // Descriptor sets
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    // Buffers
    struct FrameData {
        VkBuffer instanceBuffer = VK_NULL_HANDLE;
        VmaAllocation instanceAllocation = nullptr;
        void* instanceMapped = nullptr;

        VkBuffer cameraUBO = VK_NULL_HANDLE;
        VmaAllocation cameraAllocation = nullptr;
        void* cameraMapped = nullptr;

        VkBuffer settingsUBO = VK_NULL_HANDLE;
        VmaAllocation settingsAllocation = nullptr;
        void* settingsMapped = nullptr;
    };
    std::vector<FrameData> frameData;

    // Quad vertex buffer (shared for all sprites)
    VkBuffer quadVertexBuffer = VK_NULL_HANDLE;
    VmaAllocation quadVertexAllocation = nullptr;

    VkBuffer quadIndexBuffer = VK_NULL_HANDLE;
    VmaAllocation quadIndexAllocation = nullptr;

    // Batch data
    std::vector<SpriteInstance> spriteBatch;
    std::array<VkImageView, MAX_TEXTURES> textureViews{};
    VkSampler pixelPerfectSampler = VK_NULL_HANDLE;
    VkSampler smoothSampler = VK_NULL_HANDLE;

    uint32_t currentFrame = 0;
    bool pixelPerfectMode = true;

public:
    bool initialize(VkDevice dev, VmaAllocator alloc, VkRenderPass pass, uint32_t frameCount);
    void beginFrame(uint32_t frameIndex);
    void setCamera(const glm::mat4& view, const glm::mat4& proj, const glm::vec2& screenSize, const glm::vec2& cameraPos);
    void setRenderSettings(const FalloutRenderSettings& settings);
    void drawSprite(const glm::vec2& position, const glm::vec2& size, const glm::vec4& uvRect,
                   uint32_t textureIndex, const glm::vec4& color = glm::vec4(1.0f),
                   float rotation = 0.0f, float depth = 0.0f);
    void drawFalloutSprite(const FalloutSprite& sprite, uint32_t frameIndex, uint32_t direction,
                          const glm::vec2& position, const glm::vec2& scale = glm::vec2(1.0f),
                          const glm::vec4& color = glm::vec4(1.0f), float depth = 0.0f);
    void setTexture(uint32_t index, VkImageView textureView);
    void flush();
    void recordDrawCommands(VkCommandBuffer commandBuffer);
    void setPixelPerfectMode(bool enabled);

private:
    bool createQuadGeometry();
    bool createSamplers();
    bool createDescriptorSetLayout();
    bool createPipelineLayout();
    bool createGraphicsPipeline();
    bool createBuffers();
    bool createDescriptorPool();
    bool allocateDescriptorSets();
    void updateDescriptorSets();
    void uploadBufferData(const void* data, size_t size, VkBuffer buffer);
};

