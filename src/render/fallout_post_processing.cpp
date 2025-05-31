#include "render/fallout_post_processing.h"

namespace fallout {

bool FalloutPostProcessing::initializeForFallout(FalloutMemoryManager* memMgr,
                                                 VkDevice device,
                                                 VkAllocationCallbacks* alloc,
                                                 VkRenderPass renderPass,
                                                 uint32_t width,
                                                 uint32_t height,
                                                 uint32_t frameCount)
{
    memoryManager = memMgr;
    if (!initialize(device, alloc, renderPass, width, height, frameCount))
        return false;
    createFalloutPresets();
    return true;
}

void FalloutPostProcessing::createFalloutPresets()
{
    // Basic wasteland atmosphere
    wastelandPreset.fogColor = {0.4f, 0.3f, 0.2f};
    wastelandPreset.fogDensity = 0.05f;
    wastelandPreset.colorTemperature = 3500.0f;
    wastelandPreset.shadowTint = {0.7f, 0.8f, 1.1f};
    wastelandPreset.highlightTint = {1.2f, 1.0f, 0.8f};
    wastelandPreset.vignetteStrength = 0.3f;
    wastelandPreset.filmGrainIntensity = 0.06f;
    wastelandPreset.saturation = 0.9f;
    wastelandPreset.contrast = 1.2f;

    // Vaults tend to be cooler with artificial lighting
    vaultPreset.fogColor = {0.15f, 0.2f, 0.3f};
    vaultPreset.fogDensity = 0.03f;
    vaultPreset.colorTemperature = 5500.0f;
    vaultPreset.shadowTint = {0.6f, 0.7f, 1.3f};
    vaultPreset.highlightTint = {1.0f, 1.1f, 1.2f};
    vaultPreset.vignetteStrength = 0.4f;
    vaultPreset.filmGrainIntensity = 0.02f;

    // Crumbling city ruins
    ruinsPreset.fogColor = {0.35f, 0.35f, 0.4f};
    ruinsPreset.fogDensity = 0.04f;
    ruinsPreset.colorTemperature = 4200.0f;
    ruinsPreset.shadowTint = {0.75f, 0.8f, 1.15f};
    ruinsPreset.highlightTint = {1.1f, 1.0f, 0.9f};

    // Dark underground areas
    underworldPreset.fogColor = {0.05f, 0.05f, 0.08f};
    underworldPreset.fogDensity = 0.06f;
    underworldPreset.colorTemperature = 3000.0f;
    underworldPreset.shadowTint = {0.6f, 0.7f, 1.2f};
    underworldPreset.highlightTint = {1.1f, 1.0f, 0.8f};
    underworldPreset.vignetteStrength = 0.5f;
    underworldPreset.filmGrainIntensity = 0.07f;
}

void FalloutPostProcessing::applyWastelandPreset() { setSettings(wastelandPreset); }
void FalloutPostProcessing::applyVaultPreset() { setSettings(vaultPreset); }
void FalloutPostProcessing::applyRuinsPreset() { setSettings(ruinsPreset); }
void FalloutPostProcessing::applyUnderworldPreset() { setSettings(underworldPreset); }

} // namespace fallout
