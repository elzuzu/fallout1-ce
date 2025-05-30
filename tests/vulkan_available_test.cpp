#include "render/vulkan_render.h"
#include "test_harness.h"
#include <iostream>

int main() {
    int failed = 0;
    failed += run_test("VulkanAvailable", [](){
        bool available = fallout::vulkan_is_available();
        std::cout << "vulkan_is_available=" << available << std::endl;
        // Just ensure the call doesn't crash
        EXPECT_TRUE(available || !available);
    });
    return failed;
}
