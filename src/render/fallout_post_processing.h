#pragma once
#include "render/diablo_post_processing.h"
#include "render/vulkan_memory_manager.h"

namespace fallout {

// Extension of DiabloPostProcessing for Fallout-specific presets
class FalloutPostProcessing : public DiabloPostProcessing {
private:
    VulkanMemoryManager* memoryManager = nullptr;
    // Presets tailored for the Fallout world
    DiabloPostFXSettings wastelandPreset{};
    DiabloPostFXSettings vaultPreset{};
    DiabloPostFXSettings ruinsPreset{};
    DiabloPostFXSettings underworldPreset{};

    void createFalloutPresets();

public:
    // Initialize the post-processing system using Fallout's memory manager
    bool initializeForFallout(VulkanMemoryManager* memMgr,
                              VkDevice device,
                              VkAllocationCallbacks* alloc,
                              VkRenderPass renderPass,
                              uint32_t width,
                              uint32_t height,
                              uint32_t frameCount);

    // Apply predefined looks used across the game
    void applyWastelandPreset();
    void applyVaultPreset();
    void applyRuinsPreset();
    void applyUnderworldPreset();
};

} // namespace fallout
