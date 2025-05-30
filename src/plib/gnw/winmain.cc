#include "plib/gnw/winmain.h"

#include <stdlib.h>

#include <SDL.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "game/main.h"
#include "game/config.h"
#include "platform_compat.h"
#include "plib/gnw/gnw.h"
#include "plib/gnw/svga.h"

#if __APPLE__ && TARGET_OS_IOS
#include "platform/ios/paths.h"
#endif

namespace fallout {

// 0x53A290
bool GNW95_isActive = false;

#if _WIN32
// 0x53A294
HANDLE GNW95_mutex = NULL;
#endif

// 0x6B0760
char GNW95_title[256];

static void show_render_backend_launcher()
{
#if !defined(__ANDROID__) && !(defined(__APPLE__) && TARGET_OS_IOS)
    Config config;
    if (!config_init(&config)) {
        return;
    }

    RenderBackend backend = RenderBackend::SDL;
    if (config_load(&config, "f1_res.ini", false)) {
        char* backendName;
        if (config_get_string(&config, "MAIN", "RENDER_BACKEND", &backendName)) {
            if (compat_stricmp(backendName, "VULKAN") == 0) {
                backend = RenderBackend::VULKAN;
            }
        }
    }

    SDL_MessageBoxButtonData buttons[] = {
        { backend == RenderBackend::SDL ? SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT : 0, 0, "SDL" },
        { backend == RenderBackend::VULKAN ? SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT : 0, 1, "Vulkan" },
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Quit" },
    };

    SDL_MessageBoxData messageboxdata = {
        SDL_MESSAGEBOX_INFORMATION,
        NULL,
        "Select Rendering Backend",
        "Choose which rendering backend to use",
        SDL_arraysize(buttons),
        buttons,
        NULL
    };

    int buttonid = -1;
    if (SDL_ShowMessageBox(&messageboxdata, &buttonid) == 0) {
        const char* name = nullptr;
        switch (buttonid) {
        case 0:
            name = "SDL";
            break;
        case 1:
            name = "VULKAN";
            break;
        default:
            config_exit(&config);
            exit(0);
        }

        if (name != nullptr) {
            config_set_string(&config, "MAIN", "RENDER_BACKEND", name);
            config_save(&config, "f1_res.ini", false);
        }
    }

    config_exit(&config);
#endif
}

int main(int argc, char* argv[])
{
    int rc;

#if _WIN32
    GNW95_mutex = CreateMutexA(0, TRUE, "GNW95MUTEX");
    if (GetLastError() != ERROR_SUCCESS) {
        return 0;
    }
#endif

#if __APPLE__ && TARGET_OS_IOS
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    chdir(iOSGetDocumentsPath());
#endif

#if __APPLE__ && TARGET_OS_OSX
    char* basePath = SDL_GetBasePath();
    chdir(basePath);
    SDL_free(basePath);
#endif

#if __ANDROID__
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    chdir(SDL_AndroidGetExternalStoragePath());
#endif

    show_render_backend_launcher();

    SDL_ShowCursor(SDL_DISABLE);

    GNW95_isActive = true;
    rc = gnw_main(argc, argv);

#if _WIN32
    CloseHandle(GNW95_mutex);
#endif

    return rc;
}

} // namespace fallout

int main(int argc, char* argv[])
{
    return fallout::main(argc, argv);
}
