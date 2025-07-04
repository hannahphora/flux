#include <common/types.hpp>
#include <core/engine.hpp>

void load_functions(), load_engine(), unload_engine();
typedef void (*FluxEngineFn)(EngineState*);
FluxEngineFn engine_init_fn = nullptr;
FluxEngineFn engine_deinit_fn = nullptr;
FluxEngineFn engine_update_fn = nullptr;

#ifdef _WIN32
    #include <conio.h>
    #define WIN32_MEAN_AND_LEAN
    #include <windows.h>

    #define kbhit _kbhit
    #define getchar_unlocked _getchar_nolock

    HINSTANCE engine_dll = nullptr;

    void load_functions() {
        if (!(engine_init_fn = (FluxEngineFn)GetProcAddress(engine_dll, "init")))
            printf("failed to find init fn\n");
        if (!(engine_deinit_fn = (FluxEngineFn)GetProcAddress(engine_dll, "deinit")))
            printf("failed to find deinit fn\n");
        if (!(engine_update_fn = (FluxEngineFn)GetProcAddress(engine_dll, "update")))
            printf("failed to find update fn\n");
    }

    void load_engine() {
        if (!(engine_dll = LoadLibraryA("flux.dll")))
            printf("failed to load engine\n");
    }

    void unload_engine() {
        FreeLibrary(engine_dll);
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
                    printf("exiting\n");
                    state->running = false;
                    break;
                }
                case 'r': { // reload
                    printf("reloading\n");
                    unload_engine();
                    system("zig build");
                    load_engine();
                    load_functions();
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