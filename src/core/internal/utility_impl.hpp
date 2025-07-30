#include <common/utility.hpp>

#include <GLFW/glfw3.h>

void utility::flushDeinitStack(DeinitStack* deinitStack) {
    while (!deinitStack->empty()) {
        deinitStack->back()();
        deinitStack->pop_back();
    }
}

std::pair<u32, u32> utility::getWindowSize(const EngineState* state) {
    i32 w, h;
    glfwGetWindowSize(state->window, &w, &h);
    return { (u32)w, (u32)h };
}