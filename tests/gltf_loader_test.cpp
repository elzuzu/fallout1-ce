#include "graphics/GltfLoader.h"
#include "test_harness.h"
#include <iostream>

using namespace fallout::graphics;

int main() {
    int failed = 0;
    failed += run_test("GltfLoadSimple", [](){
        GltfLoader loader;
        ModelAsset asset;
        bool ok = loader.Load("tests/assets/simple.gltf", asset);
        EXPECT_TRUE(ok);
        EXPECT_EQ(asset.meshes.size(), 1u);
        EXPECT_EQ(asset.materials.size(), 1u);
        EXPECT_EQ(asset.animations.size(), 1u);
        EXPECT_EQ(asset.animations[0].channels.size(), 1u);
        EXPECT_EQ(asset.animations[0].channels[0].keyframes.size(), 2u);
    });
    return failed;
}
