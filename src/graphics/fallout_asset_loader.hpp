#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdint>
#include "plib/gnw/debug.h"
#include "render/vulkan_memory_manager.h"
#include <glm/vec2.hpp>
#include <algorithm>
#include <vulkan/vulkan.h>

namespace fallout {

struct FalloutSprite {
    uint32_t width = 0, height = 0;
    std::vector<uint8_t> indexedData;
    uint32_t paletteIndex = 0;
    struct AnimFrame {
        uint32_t frameIndex;
        float duration;
        int16_t offsetX;
        int16_t offsetY;
    };
    std::vector<AnimFrame> frames;
};

struct FalloutPalette {
    struct Color { uint8_t r, g, b, a; };
    Color colors[256] = {};
    std::string name;
};

class FalloutAssetLoader {
public:
    struct FRMHeader {
        uint32_t version;
        uint16_t framesPerSecond;
        uint16_t actionFrame;
        uint16_t framesPerDirection;
        uint16_t direction[6];
        uint32_t dataSize;
    };

    std::unique_ptr<FalloutSprite> loadFRM(const std::string& filename) {
        FILE* file = fopen(filename.c_str(), "rb");
        if (!file) {
            debug_printf("Failed to open FRM: %s\n", filename.c_str());
            return nullptr;
        }

        auto sprite = std::make_unique<FalloutSprite>();
        FRMHeader header{};
        if (fread(&header, sizeof(FRMHeader), 1, file) != 1) {
            fclose(file);
            debug_printf("Failed to read FRM header: %s\n", filename.c_str());
            return nullptr;
        }
        if (header.version != 4) {
            fclose(file);
            debug_printf("Unsupported FRM version: %u\n", header.version);
            return nullptr;
        }

        uint32_t frameOffset = header.direction[0];
        fseek(file, frameOffset, SEEK_SET);
        uint16_t frameWidth = 0, frameHeight = 0;
        fread(&frameWidth, sizeof(uint16_t), 1, file);
        fread(&frameHeight, sizeof(uint16_t), 1, file);
        sprite->width = frameWidth;
        sprite->height = frameHeight;
        uint32_t dataSize = frameWidth * frameHeight;
        sprite->indexedData.resize(dataSize);
        fread(sprite->indexedData.data(), 1, dataSize, file);

        if (header.framesPerDirection > 1) {
            sprite->frames.resize(header.framesPerDirection);
            float frameDuration = 1.0f / header.framesPerSecond;
            for (uint32_t i = 0; i < header.framesPerDirection; ++i) {
                sprite->frames[i].frameIndex = i;
                sprite->frames[i].duration = frameDuration;
                sprite->frames[i].offsetX = 0;
                sprite->frames[i].offsetY = 0;
            }
        }
        fclose(file);
        sprite->paletteIndex = 0;
        return sprite;
    }

    std::unique_ptr<FalloutPalette> loadPAL(const std::string& filename) {
        FILE* file = fopen(filename.c_str(), "rb");
        if (!file) {
            debug_printf("Failed to open PAL: %s\n", filename.c_str());
            return nullptr;
        }

        auto palette = std::make_unique<FalloutPalette>();
        for (int i = 0; i < 256; ++i) {
            uint8_t rgb[3];
            if (fread(rgb, 3, 1, file) != 1) {
                fclose(file);
                debug_printf("Failed to read PAL data: %s\n", filename.c_str());
                return nullptr;
            }
            palette->colors[i].r = (rgb[0] << 2) | (rgb[0] >> 4);
            palette->colors[i].g = (rgb[1] << 2) | (rgb[1] >> 4);
            palette->colors[i].b = (rgb[2] << 2) | (rgb[2] >> 4);
            palette->colors[i].a = (i == 0) ? 0 : 255;
        }
        fclose(file);
        size_t lastSlash = filename.find_last_of("/\\");
        size_t lastDot = filename.find_last_of('.');
        palette->name = filename.substr(lastSlash + 1, lastDot - lastSlash - 1);
        return palette;
    }
};

class FalloutVulkanAssets {
    VulkanMemoryManager* memoryManager = nullptr;
    std::vector<VulkanImage> spriteAtlases;
    VulkanImage paletteTexture{};
    std::vector<FalloutPalette> palettes;

public:
    bool initializePalettes(const std::vector<std::string>& paletteFiles) {
        FalloutAssetLoader loader;
        for (const auto& file : paletteFiles) {
            auto palette = loader.loadPAL(file);
            if (palette)
                palettes.push_back(*palette);
        }
        if (palettes.empty()) {
            debug_printf("No palettes loaded\n");
            return false;
        }
        std::vector<uint8_t> paletteData(256 * palettes.size() * 4);
        uint8_t* dst = paletteData.data();
        for (const auto& palette : palettes) {
            for (int i = 0; i < 256; ++i) {
                *dst++ = palette.colors[i].r;
                *dst++ = palette.colors[i].g;
                *dst++ = palette.colors[i].b;
                *dst++ = palette.colors[i].a;
            }
        }
        paletteTexture = memoryManager->createSpriteAtlas(256, palettes.size(), paletteData.data());
        return paletteTexture.image != VK_NULL_HANDLE;
    }

    uint32_t loadSpriteToAtlas(const std::string& frmFile, uint32_t atlasIndex = 0) {
        FalloutAssetLoader loader;
        auto sprite = loader.loadFRM(frmFile);
        if (!sprite) {
            debug_printf("Failed to load sprite: %s\n", frmFile.c_str());
            return UINT32_MAX;
        }
        while (atlasIndex >= spriteAtlases.size()) {
            VulkanImage newAtlas = memoryManager->createSpriteAtlas(2048, 2048, nullptr);
            spriteAtlases.push_back(newAtlas);
        }
        uint32_t spriteId = findFreeAtlasSpace(atlasIndex, sprite->width, sprite->height);
        std::vector<uint8_t> rgbaData = convertIndexedToRGBA(*sprite);
        uploadSpriteToAtlas(atlasIndex, spriteId, rgbaData.data(), sprite->width, sprite->height);
        return spriteId;
    }

private:
    std::vector<uint8_t> convertIndexedToRGBA(const FalloutSprite& sprite) {
        std::vector<uint8_t> rgba(sprite.width * sprite.height * 4);
        const auto& palette = palettes[sprite.paletteIndex];
        for (size_t i = 0; i < sprite.indexedData.size(); ++i) {
            uint8_t paletteIndex = sprite.indexedData[i];
            const auto& color = palette.colors[paletteIndex];
            rgba[i * 4 + 0] = color.r;
            rgba[i * 4 + 1] = color.g;
            rgba[i * 4 + 2] = color.b;
            rgba[i * 4 + 3] = color.a;
        }
        return rgba;
    }

    // These methods are placeholders; they should be implemented elsewhere.
    struct VulkanImage {
        VkImage image = VK_NULL_HANDLE;
    };

    uint32_t findFreeAtlasSpace(uint32_t /*atlas*/, uint32_t /*w*/, uint32_t /*h*/) { return 0; }
    VulkanImage createSpriteAtlas(uint32_t /*w*/, uint32_t /*h*/, const uint8_t* /*data*/) { return {}; }
    void uploadSpriteToAtlas(uint32_t /*atlas*/, uint32_t /*id*/, const uint8_t* /*data*/, uint32_t /*w*/, uint32_t /*h*/) {}
};

struct SpriteRenderData {
    glm::vec2 worldPos{};
    glm::vec2 scale{1.0f};
    uint32_t atlasIndex = 0;
    uint32_t spriteId = 0;
    uint32_t paletteIndex = 0;
    float depth = 0.0f;
    float alpha = 1.0f;
};

class FalloutSpriteRenderer {
    VkPipeline spritePipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VulkanBuffer spriteVertexBuffer{};
    VulkanBuffer spriteInstanceBuffer{};
    std::vector<SpriteRenderData> spriteQueue;

public:
    void queueSprite(const SpriteRenderData& sprite) { spriteQueue.push_back(sprite); }

    void renderBatch(VkCommandBuffer cmd, const glm::mat4& viewProj) {
        (void)viewProj; // unused for now
        if (spriteQueue.empty())
            return;
        std::sort(spriteQueue.begin(), spriteQueue.end(),
                   [](const SpriteRenderData& a, const SpriteRenderData& b) {
                       return a.depth < b.depth;
                   });
        updateInstanceBuffer(spriteQueue);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                               0, 1, &globalDescriptorSet, 0, nullptr);
        VkBuffer vertexBuffers[] = {spriteVertexBuffer.buffer, spriteInstanceBuffer.buffer};
        VkDeviceSize offsets[] = {0, 0};
        vkCmdBindVertexBuffers(cmd, 0, 2, vertexBuffers, offsets);
        vkCmdDraw(cmd, 6, static_cast<uint32_t>(spriteQueue.size()), 0, 0);
        spriteQueue.clear();
    }

private:
    // Placeholder types and methods
    struct VulkanBuffer { VkBuffer buffer = VK_NULL_HANDLE; };
    VkDescriptorSet globalDescriptorSet{};
    void updateInstanceBuffer(const std::vector<SpriteRenderData>&) {}
};

} // namespace fallout

