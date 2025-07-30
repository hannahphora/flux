#pragma once
#include <common/common.hpp>

#include <GLFW/glfw3.h>

namespace flux::input {
    bool init(InputState* state);
    void deinit(InputState* state);
    void update(InputState* state);
}

struct flux::InputState {
    const EngineState* engine;
    bool initialised = false;
};
