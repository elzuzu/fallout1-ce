#pragma once
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <array>
#include <string>

struct VulkanDevice;
struct VulkanQueue;
struct VulkanCommandPool;
struct VulkanRenderPass;

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VmaAllocator allocator = VK_NULL_HANDLE;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        std::optional<uint32_t> compute;
        std::optional<uint32_t> transfer;

        bool isComplete() const {
            return graphics.has_value() && present.has_value() &&
                   compute.has_value() && transfer.has_value();
        }
    } queueFamilies;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent{};
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    bool validationLayersEnabled = false;
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME
    };
};

class FalloutVulkanRenderer {
private:
    VulkanContext ctx{};

    struct FrameData {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence inFlightFence = VK_NULL_HANDLE;

        VkBuffer uniformBuffer = VK_NULL_HANDLE;
        VmaAllocation uniformAllocation = VK_NULL_HANDLE;
        void* uniformMapped = nullptr;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frames{};
    uint32_t currentFrame = 0;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    VkImage colorImage = VK_NULL_HANDLE;
    VmaAllocation colorImageAllocation = VK_NULL_HANDLE;
    VkImageView colorImageView = VK_NULL_HANDLE;
    VkImage depthImage = VK_NULL_HANDLE;
    VmaAllocation depthImageAllocation = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    mutable std::string lastError;

public:
    bool initialize(void* windowHandle, uint32_t width, uint32_t height, bool enableValidation = false);

    const char* getLastError() const { return lastError.c_str(); }

    VkDevice getDevice() const { return ctx.device; }
    VkPhysicalDevice getPhysicalDevice() const { return ctx.physicalDevice; }
    VmaAllocator getAllocator() const { return ctx.allocator; }
    VkQueue getGraphicsQueue() const { return ctx.graphicsQueue; }
    VkQueue getComputeQueue() const { return ctx.computeQueue; }
    VkQueue getTransferQueue() const { return ctx.transferQueue; }

    uint32_t getGraphicsQueueFamily() const { return ctx.queueFamilies.graphics.value(); }
    uint32_t getComputeQueueFamily() const { return ctx.queueFamilies.compute.value(); }
    uint32_t getTransferQueueFamily() const { return ctx.queueFamilies.transfer.value(); }

    VkExtent2D getSwapchainExtent() const { return ctx.swapchainExtent; }
    VkFormat getSwapchainFormat() const { return ctx.swapchainFormat; }

private:
    void setError(const std::string& error) const { lastError = error; }

    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    bool setupDebugMessenger();
    bool createSurface(void* windowHandle);
    bool createInstance();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createAllocator();
    bool createSwapchain(uint32_t width, uint32_t height);
    bool createRenderTargets();
    bool createSyncObjects();
    bool createDescriptorPool();
    bool createDescriptorSetLayout();
    bool createPipelineLayout();
    bool createFrameResources();

    int scoreDevice(VkPhysicalDevice device);
    VulkanContext::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

    VkPhysicalDeviceProperties getDeviceProperties() const {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(ctx.physicalDevice, &properties);
        return properties;
    }

    std::string getDeviceName() const {
        return std::string(getDeviceProperties().deviceName);
    }
};
