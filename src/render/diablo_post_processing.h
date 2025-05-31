#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

namespace fallout {

struct DiabloPostFXSettings {
    float exposure = 1.0f;
    float contrast = 1.1f;
    float brightness = 0.0f;
    float saturation = 1.2f;
    float gamma = 2.2f;

    glm::vec3 fogColor = glm::vec3(0.2f, 0.15f, 0.1f);
    float fogDensity = 0.02f;
    float fogStart = 10.0f;
    float fogEnd = 100.0f;

    glm::vec3 shadowTint = glm::vec3(0.8f, 0.9f, 1.1f);
    glm::vec3 highlightTint = glm::vec3(1.2f, 1.1f, 0.9f);
    float colorTemperature = 3200.0f;

    glm::vec3 sunColor = glm::vec3(1.0f, 0.8f, 0.6f);
    glm::vec3 sunDirection = glm::vec3(-0.3f, -0.8f, -0.5f);
    float scatteringIntensity = 0.5f;

    float filmGrainIntensity = 0.05f;
    float ditheringAmount = 1.0f / 255.0f;

    float vignetteStrength = 0.3f;
    float vignetteRadius = 0.8f;

    float time = 0.0f;
    float padding[3];
};

struct DiabloBloomSettings {
    float threshold = 1.0f;
    float intensity = 0.8f;
    float radius = 1.0f;
    uint32_t iterations = 5;
    float filterRadius = 1.0f;
    float padding[3];
};

class DiabloPostProcessing {
public:
    bool initialize(VkDevice dev, VkAllocationCallbacks* alloc, VkRenderPass renderPass,
                    uint32_t width, uint32_t height, uint32_t frameCount);
    void beginFrame(uint32_t frameIndex);
    void processFrame(VkCommandBuffer commandBuffer, VkImageView hdrColorView, VkImageView depthView);

    void setSettings(const DiabloPostFXSettings& newSettings);
    void setBloomSettings(const DiabloBloomSettings& newBloomSettings);
    void setAtmosphere(const glm::vec3& fogColor, float density, float start, float end);
    void setColorGrading(float exposure, float contrast, float brightness, float saturation);
    void setAtmosphericScattering(const glm::vec3& sunColor, const glm::vec3& sunDir, float intensity);

    void applyDungeonPreset();
    void applyOutdoorPreset();
    void applyCatacombsPreset();

    VkImageView getFinalOutput() const;

private:
    struct RenderTarget {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    bool createRenderTargets();
    bool createRenderTarget(RenderTarget& target, uint32_t width, uint32_t height, VkFormat format);
    void extractBloomAreas(VkCommandBuffer commandBuffer, VkImageView hdrColorView);
    void generateBloomBlur(VkCommandBuffer commandBuffer);
    void renderBloomPass(VkCommandBuffer commandBuffer, VkImageView inputView, RenderTarget& outputTarget, bool horizontal);
    void applyAtmosphericEffects(VkCommandBuffer commandBuffer, VkImageView hdrColorView, VkImageView depthView);
    void performToneMapping(VkCommandBuffer commandBuffer);
    void finalComposite(VkCommandBuffer commandBuffer);
    void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

    VkDevice device = VK_NULL_HANDLE;
    VkAllocationCallbacks* allocator = nullptr;
    VkRenderPass mainRenderPass = VK_NULL_HANDLE;

    std::array<RenderTarget, 6> bloomTargets{};
    RenderTarget hdrTarget{};
    RenderTarget depthTarget{};
    RenderTarget finalTarget{};

    VkPipeline toneMappingPipeline = VK_NULL_HANDLE;
    VkPipeline bloomExtractPipeline = VK_NULL_HANDLE;
    VkPipeline bloomBlurPipeline = VK_NULL_HANDLE;
    VkPipeline bloomCombinePipeline = VK_NULL_HANDLE;
    VkPipeline atmospherePipeline = VK_NULL_HANDLE;
    VkPipeline finalCompositePipeline = VK_NULL_HANDLE;

    VkPipelineLayout postProcessLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    struct FrameData {
        VkBuffer settingsUBO = VK_NULL_HANDLE;
        VkDeviceMemory settingsMemory = VK_NULL_HANDLE;
        VkBuffer bloomUBO = VK_NULL_HANDLE;
        VkDeviceMemory bloomMemory = VK_NULL_HANDLE;
    };
    std::vector<FrameData> frameData;
    std::vector<VkDescriptorSet> descriptorSets;

    VkSampler linearSampler = VK_NULL_HANDLE;
    VkSampler nearestSampler = VK_NULL_HANDLE;

    VkBuffer quadVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory quadVertexMemory = VK_NULL_HANDLE;

    DiabloPostFXSettings settings{};
    DiabloBloomSettings bloomSettings{};

    uint32_t screenWidth = 1920;
    uint32_t screenHeight = 1080;
    uint32_t currentFrame = 0;
};

} // namespace fallout
