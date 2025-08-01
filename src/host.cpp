#include <common/common.hpp>
#include <core/engine.hpp>

#include <common/utility.hpp>

void find_init_fn(), find_deinit_fn(), find_update_fn(), load_dl(), unload_dl();
typedef void (*DlFn)(EngineState*);
DlFn init_fn = nullptr;
DlFn deinit_fn = nullptr;
DlFn update_fn = nullptr;

#ifdef _WIN32
    #include <conio.h>
    #define WIN32_MEAN_AND_LEAN
    #include <windows.h>

    // windows alternatives for POSIXs kbhit and getchar_unlocked
    #define kbhit _kbhit
    #define getchar_unlocked _getchar_nolock

    HINSTANCE dl = nullptr;
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    void find_init_fn() { if (!(init_fn = (DlFn)GetProcAddress(dl, "init"))) fputs("failed to find init fn\n", stderr); }
    void find_deinit_fn() { if (!(deinit_fn = (DlFn)GetProcAddress(dl, "deinit"))) fputs("failed to find deinit fn\n", stderr); }
    void find_update_fn() { if (!(update_fn = (DlFn)GetProcAddress(dl, "update"))) fputs("failed to find update fn\n", stderr); }
#pragma clang diagnostic pop
    void load_dl() { if (!(dl = LoadLibraryA("flux.dll"))) fputs("failed to load dl\n", stderr); }
    void unload_dl() { if (!FreeLibrary(dl)) fputs("failed to unload dl\n", stderr); }
#else
    // TODO: implement linux support
    #error Unsupported Platform
#endif

i32 main() {
    load_dl();
    auto state = new EngineState;
    find_init_fn();
    init_fn(state);

    find_update_fn();
    state->running = true;
    while (state->running) {
        update_fn(state);
        if (kbhit()) switch (getchar_unlocked()) {
            case 'q': { // exit
                fputs("exiting\n", stdout);
                state->running = false;
                break;
            }
            case 'r': { // reload
                fputs("reloading\n", stdout);
                unload_dl();
                system("zig build");
                load_dl();
                find_update_fn();
                break;
            }
            default: break;
        }
    }

    find_deinit_fn();
    deinit_fn(state);
    delete state;
    unload_dl();
    return EXIT_SUCCESS;
}