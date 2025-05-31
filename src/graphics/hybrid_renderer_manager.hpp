#pragma once
#include <memory>
#include <string>
#include <vulkan/vulkan.h>

enum class RenderMode {
    CLASSIC_2D,
    ENHANCED_3D,
    AUTO
};

struct RenderCapabilities {
    bool supports3D = false;
    bool supportsVulkan = false;
    bool hasModernAssets = false;
    bool hasLegacyAssets = true;
    float gpuMemoryMB = 0.0f;
    std::string gpuName;
};

class FalloutHybridRenderer {
private:
    RenderMode currentMode = RenderMode::AUTO;
    RenderCapabilities capabilities;

    std::unique_ptr<DiabloGLTFLoader> gltfLoader;
    std::unique_ptr<DiabloPBRPipeline> pbrPipeline;
    std::unique_ptr<DiabloIsometricCamera> camera3D;

    std::unique_ptr<FalloutAssetLoader> assetLoader2D;
    std::unique_ptr<FalloutSpriteRenderer> spriteRenderer;
    std::unique_ptr<FalloutIsometricCamera2D> camera2D;

    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = nullptr;
    bool initialized = false;

public:
    bool initialize(VkDevice dev, VmaAllocator alloc, VkRenderPass renderPass) {
        device = dev;
        allocator = alloc;
        detectCapabilities();
        currentMode = determineOptimalMode();
        logInfo("Initializing Fallout renderer in %s mode",
                getRenderModeString(currentMode).c_str());
        switch (currentMode) {
            case RenderMode::ENHANCED_3D:
                return initialize3DRenderer(renderPass);
            case RenderMode::CLASSIC_2D:
                return initialize2DRenderer(renderPass);
            case RenderMode::AUTO:
                if (initialize3DRenderer(renderPass)) {
                    currentMode = RenderMode::ENHANCED_3D;
                    return true;
                } else {
                    logWarning("3D renderer failed, falling back to 2D mode");
                    currentMode = RenderMode::CLASSIC_2D;
                    return initialize2DRenderer(renderPass);
                }
            default:
                return false;
        }
    }

    void setRenderMode(RenderMode mode, bool forceReload = false) {
        if (mode == currentMode && !forceReload) {
            return;
        }
        logInfo("Switching render mode from %s to %s",
                getRenderModeString(currentMode).c_str(),
                getRenderModeString(mode).c_str());
        cleanup();
        currentMode = mode;
    }

    bool loadScene(const std::string& scenePath) {
        switch (currentMode) {
            case RenderMode::ENHANCED_3D:
                return load3DScene(scenePath);
            case RenderMode::CLASSIC_2D:
                return load2DScene(scenePath);
            default:
                return false;
        }
    }

    bool loadCharacter(const std::string& characterPath, uint32_t& outCharacterID) {
        switch (currentMode) {
            case RenderMode::ENHANCED_3D:
                return load3DCharacter(characterPath, outCharacterID);
            case RenderMode::CLASSIC_2D:
                return load2DCharacter(characterPath, outCharacterID);
            default:
                return false;
        }
    }

    void beginFrame() {
        switch (currentMode) {
            case RenderMode::ENHANCED_3D:
                camera3D->update(getDeltaTime());
                break;
            case RenderMode::CLASSIC_2D:
                camera2D->update(getDeltaTime());
                break;
        }
    }

    void renderCharacter(uint32_t characterID, const glm::vec2& position,
                         uint32_t animFrame = 0, uint32_t direction = 0) {
        switch (currentMode) {
            case RenderMode::ENHANCED_3D:
                render3DCharacter(characterID, position, animFrame, direction);
                break;
            case RenderMode::CLASSIC_2D:
                render2DCharacter(characterID, position, animFrame, direction);
                break;
        }
    }

    void renderEnvironment() {
        switch (currentMode) {
            case RenderMode::ENHANCED_3D:
                render3DEnvironment();
                break;
            case RenderMode::CLASSIC_2D:
                render2DEnvironment();
                break;
        }
    }

    void endFrame(VkCommandBuffer commandBuffer) {
        switch (currentMode) {
            case RenderMode::ENHANCED_3D:
                recordPBRCommands(commandBuffer);
                break;
            case RenderMode::CLASSIC_2D:
                spriteRenderer->recordDrawCommands(commandBuffer);
                break;
        }
    }

    void setCameraTarget(const glm::vec2& target) {
        switch (currentMode) {
            case RenderMode::ENHANCED_3D:
                camera3D->setTarget(glm::vec3(target.x, 0.0f, target.y));
                break;
            case RenderMode::CLASSIC_2D:
                camera2D->centerOnPoint(target);
                break;
        }
    }

    void handleInput(const InputEvent& event) {
        switch (currentMode) {
            case RenderMode::ENHANCED_3D:
                handle3DInput(event);
                break;
            case RenderMode::CLASSIC_2D:
                handle2DInput(event);
                break;
        }
    }

    struct PerformanceMetrics {
        float frameTimeMs = 0.0f;
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t sprites = 0;
        float gpuMemoryUsedMB = 0.0f;
        bool isPerformanceSufficient = true;
    };

    PerformanceMetrics getPerformanceMetrics() const {
        PerformanceMetrics metrics;
        switch (currentMode) {
            case RenderMode::ENHANCED_3D:
                metrics = get3DMetrics();
                break;
            case RenderMode::CLASSIC_2D:
                metrics = get2DMetrics();
                break;
        }
        return metrics;
    }

    void updatePerformanceAdaptation() {
        PerformanceMetrics metrics = getPerformanceMetrics();
        if (!metrics.isPerformanceSufficient && currentMode == RenderMode::ENHANCED_3D) {
            logWarning("3D performance insufficient (%.2fms frame time), considering fallback to 2D",
                       metrics.frameTimeMs);
            // Potential automatic switch could happen here
        }
    }

private:
    void detectCapabilities() {
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(getPhysicalDevice(), &deviceProps);
        capabilities.gpuName = deviceProps.deviceName;
        capabilities.supportsVulkan = true;

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(getPhysicalDevice(), &features);
        capabilities.supports3D = features.geometryShader && features.tessellationShader;

        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(getPhysicalDevice(), &memProps);
        for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
            if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                capabilities.gpuMemoryMB = memProps.memoryHeaps[i].size / (1024.0f * 1024.0f);
                break;
            }
        }

        capabilities.hasLegacyAssets = checkForFile("master.dat") || checkForFile("critter.dat");
        capabilities.hasModernAssets = checkForFile("models/") && checkForFile("textures/");

        logInfo("Detected capabilities: GPU=%s, Memory=%.0fMB, Legacy=%s, Modern=%s",
                capabilities.gpuName.c_str(), capabilities.gpuMemoryMB,
                capabilities.hasLegacyAssets ? "Yes" : "No",
                capabilities.hasModernAssets ? "Yes" : "No");
    }

    RenderMode determineOptimalMode() {
        if (capabilities.supports3D && capabilities.hasModernAssets && capabilities.gpuMemoryMB > 512.0f) {
            return RenderMode::ENHANCED_3D;
        }
        if (capabilities.hasLegacyAssets) {
            return RenderMode::CLASSIC_2D;
        }
        return RenderMode::AUTO;
    }

    bool initialize3DRenderer(VkRenderPass renderPass) {
        try {
            gltfLoader = std::make_unique<DiabloGLTFLoader>();
            pbrPipeline = std::make_unique<DiabloPBRPipeline>();
            camera3D = std::make_unique<DiabloIsometricCamera>();
            if (!pbrPipeline->create(device, renderPass)) {
                return false;
            }
            logInfo("3D renderer initialized successfully");
            return true;
        } catch (const std::exception& e) {
            logError("Failed to initialize 3D renderer: %s", e.what());
            return false;
        }
    }

    bool initialize2DRenderer(VkRenderPass renderPass) {
        try {
            assetLoader2D = std::make_unique<FalloutAssetLoader>();
            spriteRenderer = std::make_unique<FalloutSpriteRenderer>();
            camera2D = std::make_unique<FalloutIsometricCamera2D>();
            if (!assetLoader2D->initialize(device, allocator, getTransferQueue(), getTransferCommandPool())) {
                return false;
            }
            if (!spriteRenderer->initialize(device, allocator, renderPass, getFrameCount())) {
                return false;
            }
            camera2D->initialize(getScreenWidth(), getScreenHeight());
            logInfo("2D renderer initialized successfully");
            return true;
        } catch (const std::exception& e) {
            logError("Failed to initialize 2D renderer: %s", e.what());
            return false;
        }
    }

    std::string getRenderModeString(RenderMode mode) const {
        switch (mode) {
            case RenderMode::CLASSIC_2D: return "Classic 2D";
            case RenderMode::ENHANCED_3D: return "Enhanced 3D";
            case RenderMode::AUTO: return "Auto";
            default: return "Unknown";
        }
    }

    bool load3DScene(const std::string& scenePath) {
        return gltfLoader && gltfLoader->loadModel(scenePath) != nullptr;
    }

    bool load2DScene(const std::string& scenePath) {
        return true;
    }

    bool load3DCharacter(const std::string& characterPath, uint32_t& outID) {
        auto model = gltfLoader ? gltfLoader->loadModel(characterPath) : nullptr;
        if (model) {
            outID = registerCharacter3D(std::move(model));
            return true;
        }
        return false;
    }

    bool load2DCharacter(const std::string& characterPath, uint32_t& outID) {
        auto sprite = assetLoader2D ? assetLoader2D->loadSprite(characterPath) : nullptr;
        if (sprite) {
            outID = registerCharacter2D(sprite);
            return true;
        }
        return false;
    }

    uint32_t registerCharacter3D(std::unique_ptr<DiabloModel> model) { return 0; }
    uint32_t registerCharacter2D(std::shared_ptr<FalloutSprite> sprite) { return 0; }
    PerformanceMetrics get3DMetrics() const { return {}; }
    PerformanceMetrics get2DMetrics() const { return {}; }

    VkPhysicalDevice getPhysicalDevice() const; // Implementation elsewhere
    VkQueue getTransferQueue() const;
    VkCommandPool getTransferCommandPool() const;
    uint32_t getFrameCount() const;
    uint32_t getScreenWidth() const;
    uint32_t getScreenHeight() const;
    float getDeltaTime() const;
    void cleanup();
    void render3DCharacter(uint32_t, const glm::vec2&, uint32_t, uint32_t);
    void render2DCharacter(uint32_t, const glm::vec2&, uint32_t, uint32_t);
    void render3DEnvironment();
    void render2DEnvironment();
    void recordPBRCommands(VkCommandBuffer);
    bool checkForFile(const char* path) const;
    void handle3DInput(const InputEvent&);
    void handle2DInput(const InputEvent&);
    void logInfo(const char* fmt, ...) const;
    void logWarning(const char* fmt, ...) const;
    void logError(const char* fmt, ...) const;
};

class RenderModeConfig {
public:
    static RenderMode getPreferredMode() { return RenderMode::AUTO; }
    static void setPreferredMode(RenderMode mode) { (void)mode; }
    static bool getAutoFallbackEnabled() { return true; }
};
