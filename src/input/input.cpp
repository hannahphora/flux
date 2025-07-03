#include <input/input.h>

bool input::init(InputState* state) {
    state->initialised = true;
    return true;
}

bool input::deinit(InputState* state) {
    state->initialised = false;
    return true;
}
