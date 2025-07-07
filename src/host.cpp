#include <common/types.hpp>
#include <core/engine.hpp>

void load_functions(), load_engine(), unload_engine();
typedef void (*DlFn)(EngineState*);
DlFn engine_init_fn = nullptr;
DlFn engine_deinit_fn = nullptr;
DlFn engine_update_fn = nullptr;

#ifdef _WIN32
    #include <conio.h>
    #define WIN32_MEAN_AND_LEAN
    #include <windows.h>

    // kbhit and getchar_unlocked are POSIX,
    // so replace them with the windows alternatives
    #define kbhit _kbhit
    #define getchar_unlocked _getchar_nolock

    HINSTANCE engine_dl = nullptr;

    void load_functions() {
        if (!(engine_init_fn = (DlFn)GetProcAddress(engine_dl, "init")))
            fputs("failed to find init fn\n", stderr);
        if (!(engine_deinit_fn = (DlFn)GetProcAddress(engine_dl, "deinit")))
            fputs("failed to find deinit fn\n", stderr);
        if (!(engine_update_fn = (DlFn)GetProcAddress(engine_dl, "update")))
            fputs("failed to find update fn\n", stderr);
    }

    void load_engine() {
        if (!(engine_dl = LoadLibraryA("flux.dll")))
            fputs("failed to load engine\n", stderr);
    }

    void unload_engine() {
        FreeLibrary(engine_dl);
    }
#else
    // TODO: add linux support
    #error unsupported platform
#endif

i32 main() {
    load_engine();
    load_functions();
    auto state = new EngineState;
    engine_init_fn(state);

    state->running = true;
    while (state->running) {
        engine_update_fn(state);

        if (kbhit()) {
            switch (getchar_unlocked()) {
            case 'q': { // exit
                fputs("exiting\n", stdout);
                state->running = false;
                break;
            }
            case 'r': { // reload
                fputs("reloading\n", stdout);
                unload_engine();
                system("zig build");
                load_engine();
                load_functions();
                break;
            }
            case 'h': {
                fputs("host commands\n\th: help\n\tq: exit\n\tr: reload\n", stdout);
                break;
            }
            }
        }
    }

    engine_deinit_fn(state);
    delete state;
    unload_engine();
    return EXIT_SUCCESS;
}