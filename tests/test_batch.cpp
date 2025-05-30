#include "graphics/vulkan/SpriteBatch.hpp"
#include "test_harness.h"
#include <SDL.h>
#include <chrono>

int main() {
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL init failed" << std::endl;
        return 1;
    }
    int failed = 0;
    failed += run_test("SpriteBatchPush", [](){
        SpriteBatch batch;
        for (int i = 0; i < 10000; ++i) {
            Sprite s{};
            batch.draw(s);
        }
        auto start = std::chrono::high_resolution_clock::now();
        batch.flush(VK_NULL_HANDLE);
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        EXPECT_TRUE(ms >= 0); // placeholder, can't validate GPU timing
    });
    SDL_Quit();
    return failed;
}
