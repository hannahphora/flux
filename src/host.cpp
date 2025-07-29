#include <common/common.hpp>
#include <core/engine.hpp>

void load_fns(), load_dl(), unload_dl();
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

    void load_fns() {
        if (!(init_fn = (DlFn)GetProcAddress(dl, "init"))) fputs("failed to find init fn\n", stderr);
        if (!(deinit_fn = (DlFn)GetProcAddress(dl, "deinit"))) fputs("failed to find deinit fn\n", stderr);
        if (!(update_fn = (DlFn)GetProcAddress(dl, "update"))) fputs("failed to find update fn\n", stderr);
    }

    void load_dl() {
        if (!(dl = LoadLibraryA("flux.dll"))) fputs("failed to load dl\n", stderr);
    }

    void unload_dl() {
        if (!FreeLibrary(dl)) fputs("failed to unload dl\n", stderr);
    }
#else
    // TODO: add linux support
    #error unsupported platform
#endif

i32 main() {
    load_dl();
    load_fns();
    auto state = new EngineState;
    init_fn(state);

    state->running = true;
    while (state->running) {
        update_fn(state);

        if (kbhit()) {
            switch (getchar_unlocked()) {
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
                load_fns();
                break;
            }
            case 'h': {
                fputs("host commands\n\th: help\n\tq: exit\n\tr: reload\n", stdout);
                break;
            }
            }
        }
    }

    deinit_fn(state);
    delete state;
    unload_dl();
    return EXIT_SUCCESS;
}