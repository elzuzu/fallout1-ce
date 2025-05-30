#include "render/vulkan_render.h"
#include "game/graphics_advanced.h"
#include "graphics/vulkan/GraphicsPipeline3D.h"
#include "graphics/vulkan/PipelineCache.hpp"
#include "graphics/vulkan/VulkanDevice.h"
#include "graphics/vulkan/VulkanResourceAllocator.h"
#include "graphics/vulkan/VulkanSwapchain.h"
#include "plib/gnw/svga.h"
#include "render/post_processor.h"
#include "render/vulkan_capabilities.h"
#include "render/vulkan_debugger.h"
#include "graphics/vulkan/DebugHud.h"
#include "render/vulkan_thread_manager.h"
#include <cstdlib>
#include <cstring>
#include <stdexcept> // For runtime_error

#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include "game/IsometricCamera.h"
#include "game/ResourceManager.h"
#include "graphics/RenderableTypes.h"
// #include "game/object_types.h" // Not directly using Object* for rendering list yet

namespace fallout {

VulkanRenderer gVulkan;
// Global gVulkan is used by VRA's execute_single_time_commands_vra & RM's LoadTexture. Refactor needed.

// --- Global caches for GPU resources (Ideally members of VulkanRenderer) ---
// Maps a unique string (e.g., model_path + "_mesh_" + mesh_index) to VulkanMesh
std::map<std::string, VulkanMesh> gGpuMeshes;
// Maps a texture path to its loaded Vulkan TextureAsset (contains VkImage, VkImageView, VkSampler)
// ResourceManager's cache holds shared_ptr<TextureAsset>, this is for renderer convenience if needed,
// but direct use of RM::GetTexture is better. This specific global might be redundant with RM's cache.
// For now, RM will load, and we expect textures to be available via RM.GetTexture().
// std::map<std::string, std::shared_ptr<graphics::TextureAsset>> gRendererTextureCache;

// Helper function for single-time command submission (simplified)
void execute_single_time_commands(VkCommandPool pool, VkQueue queue, std::function<void(VkCommandBuffer)> recorder)
{
    // ... (implementation from previous step, assumed to be here and working) ...
    VkCommandBufferAllocateInfo allocInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(gVulkan.device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    recorder(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(gVulkan.device, pool, 1, &commandBuffer);
}

namespace { // Anonymous namespace for static helpers

    static uint32_t find_memory_type_manual(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        // ... (implementation from previous step) ...
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) return i;
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    static bool create_depth_resources() { /* ... (implementation from previous step) ... */ return true; }
    static void destroy_depth_resources() { /* ... (implementation from previous step) ... */ }
    static bool create_internal_image() { /* ... (implementation from previous step, ensure vkCreateImage, vkAllocateMemory, vkBindImageMemory, vkCreateImageView are correct) ... */ return true; }
    static void destroy_internal_image() { /* ... (implementation from previous step) ... */ }
    static bool create_swapchain(uint32_t width, uint32_t height)
    {
        (void)width;
        (void)height;
        VulkanSwapchain swap;
        if (swap.create(gVulkan.device, gVulkan.physicalDevice, gVulkan.surface, gSdlWindow, VK_NULL_HANDLE) != VK_SUCCESS) {
            return false;
        }
        gVulkan.swapchain = swap.get();
        gVulkan.swapchainImages = swap.getImages();
        gVulkan.swapchainImageViews = swap.getImageViews();
        gVulkan.swapchainImageFormat = swap.getImageFormat();
        gVulkan.swapchainExtent = swap.getExtent();
        return true;
    }
    static void destroy_swapchain() { /* ... (implementation from previous step) ... */ }

    // --- Temp: Hardcoded list of entities to render ---
    // In a real game, this list would be populated by game logic based on visibility, LODs, etc.
    std::vector<Renderable3DEntity> gVisible3DEntitiesList_Frame; // Entities for current frame

    // Helper to upload model data to GPU if not already done
    VulkanMesh* get_or_upload_gpu_mesh(const std::string& modelId, const std::string& modelCategory, int meshIdx)
    {
        if (!gVulkan.resourceManager_ || !gVulkan.resourceAllocator_) return nullptr;

        std::string gpuMeshKey = modelId + "_mesh_" + std::to_string(meshIdx);
        auto it = gGpuMeshes.find(gpuMeshKey);
        if (it != gGpuMeshes.end()) {
            return &it->second;
        }

        std::shared_ptr<graphics::ModelAsset> modelAsset = gVulkan.resourceManager_->GetModel(modelId);
        if (!modelAsset) { // Try loading if not found (e.g. if GetModel doesn't lazy load)
            modelAsset = gVulkan.resourceManager_->LoadModel(modelId, modelCategory);
        }

        if (!modelAsset || meshIdx >= modelAsset->meshes.size()) {
            fprintf(stderr, "Error: Model asset '%s' not found or mesh index %d out of bounds.\n", modelId.c_str(), meshIdx);
            return nullptr;
        }

        const auto& cpuMeshData = modelAsset->meshes[meshIdx];
        VulkanMesh newGpuMesh;
        newGpuMesh.indexCount = static_cast<uint32_t>(cpuMeshData.indices.size());
        if (!cpuMeshData.vertices.empty()) {
            // Vertex Buffer (using full graphics::Vertex)
            VkDeviceSize vbSize = cpuMeshData.vertices.size() * sizeof(graphics::Vertex);
            vk::AllocatedBuffer stagingVB;
            gVulkan.resourceAllocator_->CreateStagingBuffer(vbSize, stagingVB, true);
            memcpy(stagingVB.mappedData, cpuMeshData.vertices.data(), vbSize);
            gVulkan.resourceAllocator_->CreateVertexBuffer(vbSize, newGpuMesh.vertexBuffer, true, false);
            execute_single_time_commands(gVulkan.commandPools[0], gVulkan.graphicsQueue,
                [&](VkCommandBuffer cmd) { VkBufferCopy rgn{0,0,vbSize}; vkCmdCopyBuffer(cmd, stagingVB.buffer, newGpuMesh.vertexBuffer.buffer, 1, &rgn); });
            stagingVB.Destroy(gVulkan.resourceAllocator_->GetVmaAllocator());
        } else {
            fprintf(stderr, "Warning: Mesh %d in model '%s' has no vertex data.\n", meshIdx, modelId.c_str());
            return nullptr; // Cannot render empty mesh
        }

        if (newGpuMesh.indexCount > 0) {
            VkDeviceSize ibSize = cpuMeshData.indices.size() * sizeof(uint32_t);
            vk::AllocatedBuffer stagingIB;
            gVulkan.resourceAllocator_->CreateStagingBuffer(ibSize, stagingIB, true);
            memcpy(stagingIB.mappedData, cpuMeshData.indices.data(), ibSize);
            gVulkan.resourceAllocator_->CreateIndexBuffer(ibSize, newGpuMesh.indexBuffer, true, false);
            execute_single_time_commands(gVulkan.commandPools[0], gVulkan.graphicsQueue,
                [&](VkCommandBuffer cmd) { VkBufferCopy rgn{0,0,ibSize}; vkCmdCopyBuffer(cmd, stagingIB.buffer, newGpuMesh.indexBuffer.buffer, 1, &rgn); });
            stagingIB.Destroy(gVulkan.resourceAllocator_->GetVmaAllocator());
        }

        // Store material/texture info (simplified: just path for now)
        if (cpuMeshData.materialIndex >= 0 && cpuMeshData.materialIndex < modelAsset->materials.size()) {
            newGpuMesh.materialTexturePath = modelAsset->materials[cpuMeshData.materialIndex].baseColorTexturePath;
        }

        gGpuMeshes[gpuMeshKey] = newGpuMesh;
        std::cout << "Uploaded mesh " << meshIdx << " for model " << modelId << " to GPU." << std::endl;
        return &gGpuMeshes[gpuMeshKey];
    }

    static void record_and_submit(uint32_t imageIndex, uint32_t frameInFlightIndex, VkCommandBuffer cmdBuffer, VkFence inFlight)
    {
        vkResetCommandBuffer(cmdBuffer, 0);
        VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vkBeginCommandBuffer(cmdBuffer, &beginInfo);

        if (gGraphicsAdvanced.debugger) {
            gVulkanDebugger.begin_frame(frameInFlightIndex, cmdBuffer);
            gDebugHud.begin(cmdBuffer);
        }

        // --- Rendering setup (barriers, clear values, rendering info - as before) ---
        // (Assuming create_depth_resources and create_internal_image correctly set up attachments)
        VkClearValue clearValues[2];
        clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
        clearValues[1].depthStencil = { 1.0f, 0 };
        // ... (VkRenderingAttachmentInfo for color and depth, VkRenderingInfo offscreenInfo as before) ...
        // ... (Layout transition barriers for internal color and depth images as before) ...

        vkCmdBeginRendering(cmdBuffer, &offscreenInfo); // offscreenInfo needs to be defined and set

        // --- Iterate Entities and Render 3D ---
        if (!gVulkan.fallbackTo2D_ && gVulkan.graphicsPipeline3D_ && gVulkan.graphicsPipeline3D_->GetPipeline()) {
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gVulkan.graphicsPipeline3D_->GetPipeline());
            VkViewport viewport = { 0.0f, 0.0f, (float)gVulkan.internalExtent.width, (float)gVulkan.internalExtent.height, 0.0f, 1.0f };
            VkRect2D scissor = { { 0, 0 }, gVulkan.internalExtent };
            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

            for (const auto& entity : gVisible3DEntitiesList) { // Iterate the list from game logic
                VulkanMesh* gpuMesh = nullptr;
                std::shared_ptr<graphics::ModelAsset> modelAsset = gVulkan.resourceManager_->GetModel(entity.logicalModelName);
                if (!modelAsset) modelAsset = gVulkan.resourceManager_->LoadModel(entity.logicalModelName, entity.modelCategory);

                if (modelAsset && !modelAsset->meshes.empty()) {
                    // For simplicity, render only the first mesh of the model.
                    // A full system would iterate modelAsset->meshes.
                    gpuMesh = get_or_upload_gpu_mesh(entity.logicalModelName, entity.modelCategory, 0);
                    if (!gpuMesh) continue;

                    // Update UBO
                    SceneMatrices uboData {};
                    uboData.model = entity.worldTransform;
                    if (gVulkan.camera_) { // Ensure camera is valid
                        uboData.view = gVulkan.camera_->GetViewMatrix();
                        uboData.projection = gVulkan.camera_->GetProjectionMatrix();
                    } else {
                        continue;
                    } // No camera, cannot render
                    memcpy(gVulkan.matricesUBO_.mappedData, &uboData, sizeof(SceneMatrices));

                    // Get Texture for this mesh's material
                    std::shared_ptr<graphics::TextureAsset> textureAsset = nullptr;
                    if (!gpuMesh->materialTexturePath.empty()) {
                        textureAsset = gVulkan.resourceManager_->GetTexture(gpuMesh->materialTexturePath);
                        if (!textureAsset) textureAsset = gVulkan.resourceManager_->LoadTexture(gpuMesh->materialTexturePath, "ModelTextures");
                    }

                    // Update Combined Descriptor Set (UBO + Texture)
                    VkWriteDescriptorSet writes[2];
                    int writeCount = 0;

                    VkDescriptorBufferInfo bufferInfo { gVulkan.matricesUBO_.buffer, 0, sizeof(SceneMatrices) };
                    writes[writeCount] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                    writes[writeCount].dstSet = gVulkan.combinedDescriptorSets_[frameInFlightIndex];
                    writes[writeCount].dstBinding = 0; // UBO
                    writes[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    writes[writeCount].descriptorCount = 1;
                    writes[writeCount].pBufferInfo = &bufferInfo;
                    writeCount++;

                    VkDescriptorImageInfo imageInfo {}; // Default empty
                    if (textureAsset && textureAsset->loaded) {
                        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        imageInfo.imageView = textureAsset->imageView;
                        imageInfo.sampler = textureAsset->sampler;

                        writes[writeCount] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                        writes[writeCount].dstSet = gVulkan.combinedDescriptorSets_[frameInFlightIndex];
                        writes[writeCount].dstBinding = 1; // Texture
                        writes[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        writes[writeCount].descriptorCount = 1;
                        writes[writeCount].pImageInfo = &imageInfo;
                        writeCount++;
                    } else {
                        // Bind a dummy/default texture if no texture or failed to load?
                        // Or ensure shader handles missing texture gracefully (e.g. outputs white).
                        // For now, if no texture, binding 1 won't be updated, might use last bound texture.
                        // This should be improved by binding a default white texture.
                    }
                    vkUpdateDescriptorSets(gVulkan.device, writeCount, writes, 0, nullptr);

                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        gVulkan.graphicsPipeline3D_->GetPipelineLayout(), 0, 1,
                        &gVulkan.combinedDescriptorSets_[frameInFlightIndex], 0, nullptr);

                    VkBuffer vertexBuffers[] = { gpuMesh->vertexBuffer.buffer };
                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
                    if (gpuMesh->indexCount > 0) {
                        vkCmdBindIndexBuffer(cmdBuffer, gpuMesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                        vkCmdDrawIndexed(cmdBuffer, gpuMesh->indexCount, 1, 0, 0, 0);
                    }
                }
            }
        }
        vkCmdEndRendering(cmdBuffer);
        if (gGraphicsAdvanced.debugger) {
            gVulkanDebugger.end_frame(frameInFlightIndex, cmdBuffer);
            gDebugHud.end(cmdBuffer);
        }
        // ... (Rest of post-processing and presentation logic as before) ...
    }
} // namespace (anonymous)

bool vulkan_render_init(VideoOptions* options)
{
    // ... (SDL and instance creation done earlier) ...

    VulkanDevice deviceSelector;
    if (!deviceSelector.selectPhysicalDevice(gVulkan.instance, gVulkan.surface)) {
        return false; // No suitable GPU
    }
    gVulkan.physicalDevice = deviceSelector.getPhysicalDevice();
    if (!deviceSelector.createLogicalDevice(gVulkan.surface,
            gVulkan.device,
            gVulkan.graphicsQueue,
            gVulkan.graphicsQueueFamily)) {
        return false;
    }
    // Ensure gVulkan.resourceAllocator_, gVulkan.resourceManager_, gVulkan.camera_ are initialized.
    // This is a conceptual placeholder for where game would set these up.
    if (!gVulkan.resourceAllocator_) { /* ... fatal error ... */
        return false;
    }
    if (!gVulkan.resourceManager_ || !gVulkan.resourceManager_->Initialize("f1_res.ini" /*or actual path*/)) { /* ... fatal error ... */
        return false;
    }
    if (!gVulkan.camera_) { /* ... fatal error ... */
        return false;
    }

    if (gGraphicsAdvanced.debugger) {
        gVulkanDebugger.init(gVulkan.instance, gVulkan.physicalDevice, gVulkan.device);
        gDebugHud.init(gVulkan.instance, gVulkan.device);
    }
    if (!create_swapchain(gVulkan.width, gVulkan.height)) return false;
    // ... (Command Pools, Pipeline Cache creation) ...

    gVulkan.graphicsPipeline3D_ = new vk::GraphicsPipeline3D(gVulkan.device);
    if (!gVulkan.graphicsPipeline3D_->Create(gVulkan.swapchainImageFormat, gVulkan.pipelineCache)) {
        fprintf(stderr, "[Vulkan] Failed to create 3D pipeline, falling back to 2D sprites.\n");
        gVulkan.fallbackTo2D_ = true;
        delete gVulkan.graphicsPipeline3D_;
        gVulkan.graphicsPipeline3D_ = nullptr;
    } else {
        gVulkan.combinedDescriptorSetLayout_ = gVulkan.graphicsPipeline3D_->GetDescriptorSetLayout();
    }

    // Create/Recreate Descriptor Pool for UBOs and Textures
    // ... (Descriptor pool creation as in previous step, ensuring enough types) ...

    // Create UBO and Combined Descriptor Sets
    if (!gVulkan.fallbackTo2D_ && gVulkan.graphicsPipeline3D_ && gVulkan.resourceAllocator_) {
        gVulkan.resourceAllocator_->CreateUniformBuffer(sizeof(SceneMatrices), gVulkan.matricesUBO_, true);
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, gVulkan.combinedDescriptorSetLayout_);
        VkDescriptorSetAllocateInfo setAllocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        setAllocInfo.descriptorPool = gVulkan.descriptorPool; /* ... ensure pool is valid ... */
        setAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        setAllocInfo.pSetLayouts = layouts.data();
        gVulkan.combinedDescriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(gVulkan.device, &setAllocInfo, gVulkan.combinedDescriptorSets_.data()) != VK_SUCCESS) {
            fprintf(stderr, "[Vulkan] Failed to allocate descriptor sets, switching to 2D mode.\n");
            gVulkan.fallbackTo2D_ = true;
            gVulkan.combinedDescriptorSets_.clear();
            gVulkan.matricesUBO_.Destroy(gVulkan.resourceAllocator_->GetVmaAllocator());
        } else {
            // Initial UBO update for descriptor sets
            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                VkDescriptorBufferInfo bufferInfo { gVulkan.matricesUBO_.buffer, 0, sizeof(SceneMatrices) };
                VkWriteDescriptorSet uboWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                uboWrite.dstSet = gVulkan.combinedDescriptorSets_[i];
                uboWrite.dstBinding = 0;
                uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboWrite.descriptorCount = 1;
                uboWrite.pBufferInfo = &bufferInfo;
                // Texture part is updated per-draw in record_and_submit
                vkUpdateDescriptorSets(gVulkan.device, 1, &uboWrite, 0, nullptr);
            }
        }
    }

    // --- Pre-load some assets for testing (simulating game logic) ---
    if (!gVulkan.fallbackTo2D_) {
        // Player Character (example)
        gVisible3DEntitiesList.push_back({ "PlayerMale", "CritterModels", game::Mat4Translate({ -1.0f, 0.0f, 0.0f }) * game::Mat4Scale({ 0.5f, 0.5f, 0.5f }) });
        // Creature (example)
        gVisible3DEntitiesList.push_back({ "Radroach", "CritterModels", game::Mat4Translate({ 1.0f, 0.0f, 0.0f }) });
        // Door (example)
        // gVisible3DEntitiesList.push_back({"VaultDoor", "SceneryModels", game::Mat4Translate({0.0f, 0.0f, -2.0f}) });

        // Models (and their textures) will be loaded and GPU resources created on first sight in record_and_submit
        // via get_or_upload_gpu_mesh and RM::LoadTexture.
        // Alternatively, pre-upload them here:
        for (const auto& entityDesc : gVisible3DEntitiesList) {
            std::shared_ptr<graphics::ModelAsset> modelAsset = gVulkan.resourceManager_->LoadModel(entityDesc.logicalModelName, entityDesc.modelCategory);
            if (modelAsset) {
                for (size_t i = 0; i < modelAsset->meshes.size(); ++i) {
                    VulkanMesh* mesh = get_or_upload_gpu_mesh(entityDesc.logicalModelName, entityDesc.modelCategory, i);
                    if (mesh && !mesh->materialTexturePath.empty()) {
                        gVulkan.resourceManager_->LoadTexture(mesh->materialTexturePath, "ModelTextures");
                    }
                }
            }
        }
    }
    // ... (Semaphores, Fences, Thread start as before) ...
    return true;
}

void vulkan_render_exit()
{
    if (gVulkan.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(gVulkan.device);
        if (gGraphicsAdvanced.debugger) {
            gDebugHud.shutdown();
            gVulkanDebugger.destroy();
        }
        // ...
        if (gVulkan.resourceAllocator_) {
            gVulkan.matricesUBO_.Destroy(gVulkan.resourceAllocator_->GetVmaAllocator());
            for (auto& pair : gGpuMeshes) {
                pair.second.vertexBuffer.Destroy(gVulkan.resourceAllocator_->GetVmaAllocator());
                pair.second.indexBuffer.Destroy(gVulkan.resourceAllocator_->GetVmaAllocator());
            }
            gGpuMeshes.clear();
        }
        // Textures are managed by ResourceManager's cache which uses shared_ptrs.
        // The TextureAsset's Vulkan handles (image, view, sampler) need cleanup.
        // This should be done when the shared_ptr in RM's cache goes to zero.
        // TextureAsset would need a custom deleter or its destructor should handle it.
        // For now, assume RM's cache clearing + shared_ptr does it, or VRA handles texture cleanup.
        // The `VulkanResourceAllocator::CreateTextureImage` creates these, so VRA should destroy them.
        // This means TextureAsset's destructor should call VRA to free its image/sampler if it owns them.
        // Or, RM explicitly iterates and calls VRA->DestroyTextureImage(texAsset).

        // ... (rest of cleanup) ...
    }
    gVulkan = {};
}

// ... (vulkan_render_handle_window_size_changed, vulkan_render_present, etc. as before, no major structural changes needed there)
// record_and_submit is the main change, already shown above.
void vulkan_render_handle_window_size_changed() { /* As before */ }
void vulkan_render_present() { /* As before */ }
SDL_Surface* vulkan_render_get_surface() { return gVulkan.frameSurface; }
SDL_Surface* vulkan_render_get_texture_surface() { return gVulkan.frameTextureSurface; }
bool vulkan_is_available() { /* As before */ return true; }

} // namespace fallout
