#ifndef FALLOUT_RENDER_VULKAN_RENDER_H_
#define FALLOUT_RENDER_VULKAN_RENDER_H_

#include "plib/gnw/svga_types.h"
#include <SDL.h>
#include <vector>
#include <vulkan/vulkan.h>

#include "render/post_processor.h"
#include "graphics/vulkan/VulkanResourceAllocator.h" // For AllocatedBuffer
#include "game/GameMath.h" // For Mat4

// Forward declarations
namespace fallout {
    namespace vk { class GraphicsPipeline3D; }
    namespace game { class IsometricCamera; }
    // ResourceManager forward declaration if needed, or include its header
    class ResourceManager;
}


namespace fallout {

// Struct for MVP matrices UBO
struct SceneMatrices {
    game::Mat4 model;
    game::Mat4 view;
    game::Mat4 projection;
};

// Struct to hold Vulkan handles for a drawable mesh
struct VulkanMesh {
    vk::AllocatedBuffer vertexBuffer;
    vk::AllocatedBuffer indexBuffer;
    uint32_t indexCount = 0;
    // Potentially VkDescriptorSet for textures if used (currently combined in matrixDescriptorSets_)
    std::string materialTexturePath; // Keep track of what texture it uses
};

// Simplified structure for game logic to pass to renderer
struct Renderable3DEntity {
    std::string logicalModelName; // e.g., "PlayerMale", "Radroach"
    std::string modelCategory;    // e.g., "CritterModels", "SceneryModels"
    game::Mat4 worldTransform;
    // If an entity can have multiple meshes, this might be more complex,
    // or the renderer iterates meshes from the ModelAsset.
};


class VulkanRenderer {
public:
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapchainExtent {};
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkCommandPool> commandPools;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkSemaphore> renderFinished;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE; // General pool, might need specific one for UBOs/textures
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    VkImage internalImage = VK_NULL_HANDLE;
    VkDeviceMemory internalImageMemory = VK_NULL_HANDLE;
    VkImageView internalImageView = VK_NULL_HANDLE;
    VkExtent2D internalExtent {};
    SDL_Surface* frameSurface = nullptr;
    SDL_Surface* frameTextureSurface = nullptr;
    PostProcessor postProcessor;

    // 3D Rendering Specifics
    vk::GraphicsPipeline3D* graphicsPipeline3D_ = nullptr;
    bool fallbackTo2D_ = false;

    // VulkanMesh testMesh_; // Replaced by gGpuMeshes map (though gGpuMeshes is currently global for simplicity)
    vk::AllocatedBuffer matricesUBO_; // One UBO, updated per draw
    VkDescriptorSetLayout combinedDescriptorSetLayout_ = VK_NULL_HANDLE; // Layout for UBO + Texture
    std::vector<VkDescriptorSet> combinedDescriptorSets_; // One per frame in flight, updated per draw

    // Depth Buffer for 3D rendering
    VkImage depthImage_ = VK_NULL_HANDLE;
    VmaAllocation depthImageAllocation_ = VK_NULL_HANDLE;
    VkImageView depthImageView_ = VK_NULL_HANDLE;
    VkFormat depthImageFormat_ = VK_FORMAT_D32_SFLOAT; // Default, should be checked for support

    // Pointers to external systems (to be set up by game logic)
    vk::VulkanResourceAllocator* resourceAllocator_ = nullptr;
    game::IsometricCamera* camera_ = nullptr;
    ResourceManager* resourceManager_ = nullptr; // For loading the model asset
};

extern VulkanRenderer gVulkan; // Global Vulkan context

// Initializes the Vulkan renderer. Returns true on success.
bool vulkan_render_init(VideoOptions* options);

// Cleans up Vulkan renderer resources.
void vulkan_render_exit();

// Handles window size change for Vulkan backend.
void vulkan_render_handle_window_size_changed();

// Presents the current frame using Vulkan.
void vulkan_render_present();

// Accessors for the software surfaces used by the engine when rendering with
// the Vulkan backend.
SDL_Surface* vulkan_render_get_surface();
SDL_Surface* vulkan_render_get_texture_surface();

// Returns true if Vulkan support is available on the system.
bool vulkan_is_available();

} // namespace fallout

#endif /* FALLOUT_RENDER_VULKAN_RENDER_H_ */
