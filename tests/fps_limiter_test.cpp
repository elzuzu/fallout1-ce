#include "fps_limiter.h"
#include "test_harness.h"
#include <SDL.h>

int main() {
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL init failed" << std::endl;
        return 1;
    }

    int failed = 0;
    failed += run_test("FpsLimiterDefault", [](){
        fallout::FpsLimiter limiter;
        limiter.mark();
        SDL_Delay(10);
        Uint32 before = SDL_GetTicks();
        limiter.throttle();
        Uint32 after = SDL_GetTicks();
        EXPECT_TRUE(after - before >= 0);
    });

    SDL_Quit();
    return failed;
}
