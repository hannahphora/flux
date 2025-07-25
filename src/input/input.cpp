#include <input/input.hpp>
#include <core/engine.hpp>

bool input::init(InputState* state) {
    state->initialised = true;
    return true;
}

void input::deinit(InputState* state) {
    state->initialised = false;
}

void input::update(InputState* state) {

}