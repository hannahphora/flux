#include <input/input.hpp>

bool input::init(InputState* state) {
    state->initialised = true;
    return true;
}

void input::deinit(InputState* state) {
    state->initialised = false;
}
