#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include "plib/gnw/debug.h"
#include "render/vulkan_memory_manager.h"

namespace fallout {

struct FalloutFrame {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> indexedData;
    int16_t offsetX = 0;
    int16_t offsetY = 0;
    float duration = 0.0f;
};

struct FalloutSprite {
    std::vector<FalloutFrame> frames;
    uint32_t paletteIndex = 0;
    std::string filename;
    bool isAnimated() const { return frames.size() > 1; }
};

struct FalloutPalette {
    struct Color { uint8_t r, g, b, a; };
    Color colors[256]{};
    std::string name;
    uint32_t index = 0;
};

class FalloutAssetLoader {
public:
    struct FRMHeader {
        uint32_t version;
        uint16_t framesPerSecond;
        uint16_t actionFrame;
        uint16_t framesPerDirection;
        uint16_t shiftX[6];
        uint16_t shiftY[6];
        uint32_t dataOffset[6];
        uint32_t dataSize;
    };

    struct FrameHeader {
        uint16_t width;
        uint16_t height;
        uint16_t frameCount;
        uint16_t pixelsPerFrame;
    };

    std::unique_ptr<FalloutSprite> loadFRM(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            debug_printf("Cannot open FRM file: %s\n", filename.c_str());
            return nullptr;
        }

        auto sprite = std::make_unique<FalloutSprite>();
        sprite->filename = filename;

        FRMHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(FRMHeader));
        if (file.gcount() != static_cast<std::streamsize>(sizeof(FRMHeader))) {
            debug_printf("Invalid FRM header in file: %s\n", filename.c_str());
            return nullptr;
        }

        if (header.version != 4) {
            debug_printf("Unsupported FRM version %u in file: %s\n", header.version, filename.c_str());
            return nullptr;
        }

        if (header.framesPerDirection == 0) {
            debug_printf("Invalid frames per direction in FRM: %s\n", filename.c_str());
            return nullptr;
        }

        uint32_t directionIndex = 0;
        if (header.dataOffset[directionIndex] == 0) {
            debug_printf("No data for direction 0 in FRM: %s\n", filename.c_str());
            return nullptr;
        }

        file.seekg(header.dataOffset[directionIndex]);

        FrameHeader frameHeader{};
        file.read(reinterpret_cast<char*>(&frameHeader), sizeof(FrameHeader));

        if (frameHeader.width == 0 || frameHeader.height == 0) {
            debug_printf("Invalid frame dimensions in FRM: %s\n", filename.c_str());
            return nullptr;
        }

        float frameDuration = (header.framesPerSecond > 0) ? (1.0f / header.framesPerSecond) : 0.1f;

        sprite->frames.reserve(header.framesPerDirection);
        for (uint16_t frame = 0; frame < header.framesPerDirection; ++frame) {
            FalloutFrame falloutFrame{};
            falloutFrame.width = frameHeader.width;
            falloutFrame.height = frameHeader.height;
            falloutFrame.offsetX = header.shiftX[directionIndex];
            falloutFrame.offsetY = header.shiftY[directionIndex];
            falloutFrame.duration = frameDuration;

            uint32_t pixelCount = frameHeader.width * frameHeader.height;
            falloutFrame.indexedData.resize(pixelCount);
            file.read(reinterpret_cast<char*>(falloutFrame.indexedData.data()), pixelCount);
            if (file.gcount() != static_cast<std::streamsize>(pixelCount)) {
                debug_printf("Incomplete pixel data for frame %u in FRM: %s\n", frame, filename.c_str());
                return nullptr;
            }

            sprite->frames.push_back(std::move(falloutFrame));
        }

        sprite->paletteIndex = 0;

        debug_printf("Loaded FRM: %s (%ux%u, %u frames)\n", filename.c_str(), frameHeader.width, frameHeader.height, header.framesPerDirection);
        return sprite;
    }

    std::unique_ptr<FalloutPalette> loadPAL(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            debug_printf("Cannot open PAL file: %s\n", filename.c_str());
            return nullptr;
        }

        auto palette = std::make_unique<FalloutPalette>();
        size_t lastSlash = filename.find_last_of("/\\");
        size_t lastDot = filename.find_last_of('.');
        if (lastDot != std::string::npos && lastDot > lastSlash)
            palette->name = filename.substr(lastSlash + 1, lastDot - lastSlash - 1);
        else
            palette->name = "unknown";

        for (int i = 0; i < 256; ++i) {
            uint8_t rgb[3];
            file.read(reinterpret_cast<char*>(rgb), 3);
            if (file.gcount() != 3) {
                debug_printf("Incomplete palette data in file: %s\n", filename.c_str());
                return nullptr;
            }
            palette->colors[i].r = (rgb[0] << 2) | (rgb[0] >> 4);
            palette->colors[i].g = (rgb[1] << 2) | (rgb[1] >> 4);
            palette->colors[i].b = (rgb[2] << 2) | (rgb[2] >> 4);
            palette->colors[i].a = (i == 0) ? 0 : 255;
        }

        debug_printf("Loaded PAL: %s\n", palette->name.c_str());
        return palette;
    }

    struct ColorCycling {
        uint8_t start;
        uint8_t end;
        float speed;
        bool reverse;
    };

    std::vector<ColorCycling> loadColorCycles(const std::string& filename)
    {
        std::vector<ColorCycling> cycles;
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
            return cycles;

        ColorCycling cycle{};
        while (file.read(reinterpret_cast<char*>(&cycle), sizeof(ColorCycling))) {
            if (cycle.start < cycle.end && cycle.end < 256)
                cycles.push_back(cycle);
        }
        debug_printf("Loaded %zu color cycles from: %s\n", cycles.size(), filename.c_str());
        return cycles;
    }
};

class FalloutVulkanAssets {
private:
    VulkanMemoryManager* memoryManager = nullptr;
    std::vector<FalloutPalette> palettes;
    VulkanImage paletteTexture{};
    std::vector<VulkanImage> spriteAtlases;

    struct AtlasNode {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        bool occupied = false;
        std::unique_ptr<AtlasNode> child[2];
    };
    std::vector<std::unique_ptr<AtlasNode>> atlasRoots;

    struct SpriteEntry {
        uint32_t atlasIndex = 0;
        uint32_t spriteId = 0;
        uint32_t frameCount = 0;
        glm::vec4 uvRect{};
        glm::vec2 size{};
        glm::vec2 offset{};
    };
    std::unordered_map<std::string, SpriteEntry> spriteRegistry;

public:
    bool initialize(VulkanMemoryManager* memMgr)
    {
        memoryManager = memMgr;
        return true;
    }

    bool loadPalettes(const std::vector<std::string>& paletteFiles)
    {
        FalloutAssetLoader loader;
        for (size_t i = 0; i < paletteFiles.size(); ++i) {
            auto palette = loader.loadPAL(paletteFiles[i]);
            if (palette) {
                palette->index = static_cast<uint32_t>(i);
                palettes.push_back(*palette);
            } else {
                debug_printf("Failed to load palette: %s\n", paletteFiles[i].c_str());
            }
        }

        if (palettes.empty()) {
            debug_printf("No palettes loaded\n");
            return false;
        }
        return createPaletteTexture();
    }

    uint32_t loadSprite(const std::string& frmFile, uint32_t paletteIndex = 0)
    {
        auto it = spriteRegistry.find(frmFile);
        if (it != spriteRegistry.end())
            return it->second.spriteId;

        FalloutAssetLoader loader;
        auto sprite = loader.loadFRM(frmFile);
        if (!sprite) {
            debug_printf("Failed to load sprite: %s\n", frmFile.c_str());
            return UINT32_MAX;
        }

        if (paletteIndex >= palettes.size())
            paletteIndex = sprite->paletteIndex;

        if (paletteIndex >= palettes.size()) {
            debug_printf("Invalid palette index %u for sprite: %s\n", paletteIndex, frmFile.c_str());
            return UINT32_MAX;
        }

        const FalloutFrame& firstFrame = sprite->frames[0];
        std::vector<uint8_t> rgbaData = convertIndexedToRGBA(firstFrame, palettes[paletteIndex]);

        uint32_t atlasIndex = 0, spriteId = 0;
        glm::vec4 uvRect{};
        if (!allocateAtlasSpace(firstFrame.width, firstFrame.height, atlasIndex, spriteId, uvRect)) {
            debug_printf("No space in atlas for sprite: %s\n", frmFile.c_str());
            return UINT32_MAX;
        }

        uploadSpriteToAtlas(atlasIndex, uvRect, rgbaData.data(), firstFrame.width, firstFrame.height);

        SpriteEntry entry;
        entry.atlasIndex = atlasIndex;
        entry.spriteId = spriteId;
        entry.frameCount = static_cast<uint32_t>(sprite->frames.size());
        entry.uvRect = uvRect;
        entry.size = glm::vec2(firstFrame.width, firstFrame.height);
        entry.offset = glm::vec2(firstFrame.offsetX, firstFrame.offsetY);
        spriteRegistry[frmFile] = entry;

        debug_printf("Loaded sprite: %s (atlas %u, id %u)\n", frmFile.c_str(), atlasIndex, spriteId);
        return spriteId;
    }

private:
    bool createPaletteTexture()
    {
        if (palettes.empty())
            return false;

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

        paletteTexture = memoryManager->createSpriteAtlas(256, static_cast<uint32_t>(palettes.size()), paletteData.data());
        return paletteTexture.image != VK_NULL_HANDLE;
    }

    std::vector<uint8_t> convertIndexedToRGBA(const FalloutFrame& frame, const FalloutPalette& palette)
    {
        std::vector<uint8_t> rgbaData(frame.width * frame.height * 4);
        for (size_t i = 0; i < frame.indexedData.size(); ++i) {
            uint8_t paletteIdx = frame.indexedData[i];
            const auto& color = palette.colors[paletteIdx];
            size_t idx = i * 4;
            rgbaData[idx + 0] = color.r;
            rgbaData[idx + 1] = color.g;
            rgbaData[idx + 2] = color.b;
            rgbaData[idx + 3] = color.a;
        }
        return rgbaData;
    }

    bool allocateAtlasSpace(uint32_t width, uint32_t height, uint32_t& atlasIndex, uint32_t& spriteId, glm::vec4& uvRect)
    {
        for (size_t i = 0; i < atlasRoots.size(); ++i) {
            if (findAtlasSpace(atlasRoots[i].get(), width, height, uvRect)) {
                atlasIndex = static_cast<uint32_t>(i);
                spriteId = generateSpriteId();
                return true;
            }
        }

        constexpr uint32_t ATLAS_SIZE = 2048;
        VulkanImage newAtlas = memoryManager->createSpriteAtlas(ATLAS_SIZE, ATLAS_SIZE, nullptr);
        if (newAtlas.image == VK_NULL_HANDLE)
            return false;

        spriteAtlases.push_back(newAtlas);
        auto root = std::make_unique<AtlasNode>();
        root->width = ATLAS_SIZE;
        root->height = ATLAS_SIZE;
        atlasRoots.push_back(std::move(root));

        if (findAtlasSpace(atlasRoots.back().get(), width, height, uvRect)) {
            atlasIndex = static_cast<uint32_t>(spriteAtlases.size() - 1);
            spriteId = generateSpriteId();
            return true;
        }
        return false;
    }

    bool findAtlasSpace(AtlasNode* node, uint32_t width, uint32_t height, glm::vec4& uvRect)
    {
        if (!node)
            return false;

        if (node->occupied)
            return findAtlasSpace(node->child[0].get(), width, height, uvRect) || findAtlasSpace(node->child[1].get(), width, height, uvRect);

        if (width > node->width || height > node->height)
            return false;

        if (width == node->width && height == node->height) {
            node->occupied = true;
            uvRect = glm::vec4(node->x, node->y, width, height);
            return true;
        }

        node->child[0] = std::make_unique<AtlasNode>();
        node->child[1] = std::make_unique<AtlasNode>();

        uint32_t dw = node->width - width;
        uint32_t dh = node->height - height;
        if (dw > dh) {
            node->child[0]->x = node->x;
            node->child[0]->y = node->y;
            node->child[0]->width = width;
            node->child[0]->height = node->height;

            node->child[1]->x = node->x + width;
            node->child[1]->y = node->y;
            node->child[1]->width = node->width - width;
            node->child[1]->height = node->height;
        } else {
            node->child[0]->x = node->x;
            node->child[0]->y = node->y;
            node->child[0]->width = node->width;
            node->child[0]->height = height;

            node->child[1]->x = node->x;
            node->child[1]->y = node->y + height;
            node->child[1]->width = node->width;
            node->child[1]->height = node->height - height;
        }
        return findAtlasSpace(node->child[0].get(), width, height, uvRect);
    }

    uint32_t generateSpriteId()
    {
        static uint32_t nextId = 1;
        return nextId++;
    }

    void uploadSpriteToAtlas(uint32_t atlasIndex, const glm::vec4& uvRect, const uint8_t* data, uint32_t width, uint32_t height)
    {
        (void)atlasIndex;
        (void)uvRect;
        (void)data;
        (void)width;
        (void)height;
        // Real implementation would upload pixel data via staging buffers.
    }
};

struct SpriteRenderData {
    glm::vec2 worldPos{};
    glm::vec2 scale{1.0f};
    float rotation = 0.0f;
    uint32_t spriteId = 0;
    uint32_t atlasIndex = 0;
    uint32_t paletteIndex = 0;
    float depth = 0.0f;
    float alpha = 1.0f;
    glm::vec4 tint{1.0f};
};

class FalloutSpriteRenderer {
private:
    VkPipeline spritePipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VulkanBuffer quadVertexBuffer{};
    VulkanBuffer instanceBuffer{};
    std::vector<SpriteRenderData> spriteQueue;
    std::vector<SpriteRenderData> transparentQueue;
    static constexpr uint32_t MAX_SPRITES_PER_BATCH = 1000;

public:
    bool initialize(VkDevice device, VkRenderPass renderPass)
    {
        (void)device;
        (void)renderPass;
        return createQuadVertexBuffer() && createInstanceBuffer() && createDescriptorSetLayout() && createPipeline();
    }

    void queueSprite(const SpriteRenderData& sprite)
    {
        if (sprite.alpha < 1.0f || sprite.tint.a < 1.0f)
            transparentQueue.push_back(sprite);
        else
            spriteQueue.push_back(sprite);
    }

    void renderBatch(VkCommandBuffer cmd, const glm::mat4& viewProj)
    {
        renderSpriteQueue(cmd, viewProj, spriteQueue, false);
        std::sort(transparentQueue.begin(), transparentQueue.end(), [](const SpriteRenderData& a, const SpriteRenderData& b) { return a.depth > b.depth; });
        renderSpriteQueue(cmd, viewProj, transparentQueue, true);
        spriteQueue.clear();
        transparentQueue.clear();
    }

private:
    void renderSpriteQueue(VkCommandBuffer cmd, const glm::mat4& /*viewProj*/, const std::vector<SpriteRenderData>& queue, bool /*enableBlending*/)
    {
        if (queue.empty())
            return;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
        VkBuffer vertexBuffers[] = {quadVertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

        size_t processed = 0;
        while (processed < queue.size()) {
            size_t batchSize = std::min(queue.size() - processed, static_cast<size_t>(MAX_SPRITES_PER_BATCH));
            updateInstanceBuffer(queue.data() + processed, batchSize);
            VkBuffer instanceBuffers[] = {instanceBuffer.buffer};
            vkCmdBindVertexBuffers(cmd, 1, 1, instanceBuffers, offsets);
            vkCmdDraw(cmd, 6, static_cast<uint32_t>(batchSize), 0, 0);
            processed += batchSize;
        }
    }

    // Placeholder helpers
    bool createQuadVertexBuffer() { return true; }
    bool createInstanceBuffer() { return true; }
    bool createDescriptorSetLayout() { return true; }
    bool createPipeline() { return true; }
    void updateInstanceBuffer(const SpriteRenderData* /*data*/, size_t /*count*/) {}
};

} // namespace fallout


