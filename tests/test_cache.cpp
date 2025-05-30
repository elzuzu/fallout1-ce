#include "graphics/vulkan/PipelineCache.hpp"
#include "render/vulkan_render.h"
#include "test_harness.h"
#include <SDL.h>
#include <chrono>
#include <iostream>

using namespace fallout;

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL init failed" << std::endl;
        return 1;
    }

    int failed = 0;
    failed += run_test("PipelineCacheSpeed", [](){
        if (!vulkan_is_available()) {
            std::cout << "Vulkan not available, skipping" << std::endl;
            return;
        }
        VkInstance inst;
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        VkInstanceCreateInfo instInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        instInfo.pApplicationInfo = &app;
        if (vkCreateInstance(&instInfo, nullptr, &inst) != VK_SUCCESS) {
            std::cout << "Instance creation failed, skipping" << std::endl;
            return;
        }
        uint32_t gpuCount = 0;
        vkEnumeratePhysicalDevices(inst, &gpuCount, nullptr);
        if (gpuCount == 0) {
            vkDestroyInstance(inst, nullptr);
            std::cout << "No GPU found, skipping" << std::endl;
            return;
        }
        std::vector<VkPhysicalDevice> gpus(gpuCount);
        vkEnumeratePhysicalDevices(inst, &gpuCount, gpus.data());
        VkPhysicalDevice gpu = gpus[0];

        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families.data());
        uint32_t familyIndex = 0;
        for (uint32_t i = 0; i < familyCount; ++i) {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                familyIndex = i;
                break;
            }
        }
        float priority = 1.0f;
        VkDeviceQueueCreateInfo qinfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        qinfo.queueFamilyIndex = familyIndex;
        qinfo.queueCount = 1;
        qinfo.pQueuePriorities = &priority;
        VkDeviceCreateInfo dinfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        dinfo.queueCreateInfoCount = 1;
        dinfo.pQueueCreateInfos = &qinfo;
        VkDevice device;
        if (vkCreateDevice(gpu, &dinfo, nullptr, &device) != VK_SUCCESS) {
            vkDestroyInstance(inst, nullptr);
            std::cout << "Device creation failed, skipping" << std::endl;
            return;
        }

        PipelineCache::init(device, "cache/test_pipeline.bin");
        VkPipelineCache cache = PipelineCache::handle();

        VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        VkPipelineLayout layout;
        vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout);

        // Dummy shader code (SPIR-V for "void main(){}")
        const uint32_t shaderCode[] = {
            0x07230203,0x00010000,0x00080001,0x00000006,
            0x00000000,0x00020011,0x00000001,0x0006000F,
            0x00000005,0x00000004,0x6E69616D,0x00000000,
            0x00000000,0x00030003,0x00000002,0x00000100,
            0x00030005,0x00000004,0x006E6961,0x00040047,
            0x00000004,0x0000001E,0x00000000,0x00020013,
            0x00000002,0x00030021,0x00000003,0x00000002,
            0x00030016,0x00000005,0x00000020,0x00050036,
            0x00000002,0x00000004,0x00000000,0x00000003,
            0x000200F8,0x00000001,0x000100FD,0x00010038
        };
        VkShaderModuleCreateInfo smInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        smInfo.codeSize = sizeof(shaderCode);
        smInfo.pCode = shaderCode;
        VkShaderModule module;
        vkCreateShaderModule(device, &smInfo, nullptr, &module);

        VkComputePipelineCreateInfo cpInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        cpInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        cpInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        cpInfo.stage.module = module;
        cpInfo.stage.pName = "main";
        cpInfo.layout = layout;

        auto measure = [&](VkPipelineCache c){
            auto start = std::chrono::high_resolution_clock::now();
            for(int i=0;i<100;i++){
                VkPipeline p;
                vkCreateComputePipelines(device, c, 1, &cpInfo, nullptr, &p);
                vkDestroyPipeline(device, p, nullptr);
            }
            auto end = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
        };

        long first = measure(VK_NULL_HANDLE);
        PipelineCache::shutdown();
        PipelineCache::init(device, "cache/test_pipeline.bin");
        cache = PipelineCache::handle();
        long second = measure(cache);

        std::cout << "first=" << first << "us second=" << second << "us" << std::endl;
        EXPECT_TRUE(second * 10 < first || first <= 0);

        vkDestroyShaderModule(device, module, nullptr);
        vkDestroyPipelineLayout(device, layout, nullptr);
        PipelineCache::shutdown();
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(inst, nullptr);
    });

    SDL_Quit();
    return failed;
}
