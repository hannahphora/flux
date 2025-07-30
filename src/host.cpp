#include <common/common.hpp>
#include <core/engine.hpp>

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
    void find_init_fn() { if (!(init_fn = (DlFn)GetProcAddress(dl, "init"))) fputs("failed to find init fn\n", stderr); }
    void find_deinit_fn() { if (!(deinit_fn = (DlFn)GetProcAddress(dl, "deinit"))) fputs("failed to find deinit fn\n", stderr); }
    void find_update_fn() { if (!(update_fn = (DlFn)GetProcAddress(dl, "update"))) fputs("failed to find update fn\n", stderr); }
    void load_dl() { if (!(dl = LoadLibraryA("flux.dll"))) fputs("failed to load dl\n", stderr); }
    void unload_dl() { if (!FreeLibrary(dl)) fputs("failed to unload dl\n", stderr); }
#else
    #include <dlfcn.h>
    #include <stdio.h>
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>

    // POSIX has getchar_unlocked already
    #define kbhit  posix_kbhit
    #define getchar_unlocked getchar_unlocked

    static struct termios _old_termios;
    static bool _termios_initialized = false;

    // set terminal to raw, non‐blocking mode on first use
    static void ensure_raw_mode() {
        if (_termios_initialized) return;
        struct termios newt;
        if (tcgetattr(STDIN_FILENO, &_old_termios) == -1) {
            perror("tcgetattr");
            return;
        }
        newt = _old_termios;
        newt.c_lflag &= ~(ICANON | ECHO);           // disable canonical & echo
        newt.c_cc[VMIN]  = 0;                       // read() returns even if no char
        newt.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1)
            perror("tcsetattr");
        // make read non‐blocking
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        _termios_initialized = true;
    }

    // return non‐zero if a key is waiting
    static int posix_kbhit(void) {
        ensure_raw_mode();
        int c = getchar_unlocked();
        if (c != EOF) {
            ungetc(c, stdin);
            return 1;
        }
        return 0;
    }

    // restore terminal state when you’re done
    static void restore_terminal() {
        if (!_termios_initialized) return;
        tcsetattr(STDIN_FILENO, TCSANOW, &_old_termios);
        _termios_initialized = false;
    }

    // POSIX alternatives for loading flux.dll
    void* dl = nullptr;
    void find_init_fn() {
        init_fn = (DlFn)dlsym(dl, "init");
        if (!init_fn) fputs(dlerror(), stderr);
    }
    void find_deinit_fn() {
        deinit_fn = (DlFn)dlsym(dl, "deinit");
        if (!deinit_fn) fputs(dlerror(), stderr);
    }
    void find_update_fn() {
        update_fn = (DlFn)dlsym(dl, "update");
        if (!update_fn) fputs(dlerror(), stderr);
    }
    void load_dl() {
        // conventionally on Linux your library will be libflux.so
        dl = dlopen("libflux.so", RTLD_NOW);
        if (!dl) fputs(dlerror(), stderr);
    }
    void unload_dl() {
        if (dl) {
            if (dlclose(dl) != 0)
                fputs(dlerror(), stderr);
            dl = nullptr;
        }
    }
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