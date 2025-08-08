#include <common/common.hpp>
#include <core/engine.hpp>
#include <common/utility.hpp>

#ifdef HOT_RELOADING
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

        #ifdef HOT_RELOADING
            HINSTANCE dl = nullptr;
            #pragma clang diagnostic push
            #pragma GCC diagnostic ignored "-Wcast-function-type"
            void find_init_fn() { if (!(init_fn = (DlFn)GetProcAddress(dl, "init"))) fputs("failed to find init fn\n", stderr); }
            void find_deinit_fn() { if (!(deinit_fn = (DlFn)GetProcAddress(dl, "deinit"))) fputs("failed to find deinit fn\n", stderr); }
            void find_update_fn() { if (!(update_fn = (DlFn)GetProcAddress(dl, "update"))) fputs("failed to find update fn\n", stderr); }
            #pragma clang diagnostic pop
            void load_dl() { if (!(dl = LoadLibraryA("flux.dll"))) fputs("failed to load dl\n", stderr); }
            void unload_dl() { if (!FreeLibrary(dl)) fputs("failed to unload dl\n", stderr); }
        #endif // HOT_RELOADING
    #else
        // TODO: implement linux support
        #error Unsupported Platform
    #endif
#else
    #define init_fn     engine::init
    #define deinit_fn   engine::deinit
    #define update_fn   engine::update
#endif // HOT_RELOADING

i32 main() {

#ifdef HOT_RELOADING
    load_dl();
    find_init_fn();
    find_update_fn();
#endif // HOT_RELOADING

    while (!kbhit());

    auto state = new EngineState;
    init_fn(state);
    state->running = true;
    while (state->running) {
        update_fn(state);
        
    #ifdef HOT_RELOADING
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
    #endif // HOT_RELOADING
    }

#ifdef HOT_RELOADING
    find_deinit_fn();
    unload_dl();
#endif // HOT_RELOADING

    deinit_fn(state);
    delete state;
    return EXIT_SUCCESS;
}