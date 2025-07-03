#pragma once
#include <common.h>

#include <GLFW/glfw3.h>

namespace flux {
//--------------------------------------------------------------------------------------------

struct InputState {
    const EngineState* engine;
    bool initialised = false;
};

namespace input {
    bool init(InputState* state);
    void deinit(InputState* state);
};

//--------------------------------------------------------------------------------------------
};