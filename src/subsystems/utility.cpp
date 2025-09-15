#include "utility.hpp"

#include <core/engine.hpp>
#include <subsystems/log.hpp>

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

std::pair<u32, u32> utility::getMonitorRes(const EngineState* state) {
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    return { (u32)mode->width, (u32)mode->height };
}

void utility::sleepMs(const u32 ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void utility::abort() {
    log::flush();
    std::abort();
}

void utility::exitWithFailure() {
    log::flush();
    std::exit(EXIT_FAILURE);
}

void utility::exitWithSuccess() {
    log::flush();
    std::exit(EXIT_SUCCESS);
}