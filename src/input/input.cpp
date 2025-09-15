#include "input.hpp"

#include <core/engine.hpp>

using namespace input;

bool input::init(InputState* state) {
    state->initialised = true;
    return true;
}

void input::deinit(InputState* state) {
    state->initialised = false;
}

void input::update(InputState* state) {
    if (glfwGetKey(state->engine->window, GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(state->engine->window, true);
}