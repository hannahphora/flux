#pragma once
#include <common.hpp>

// silence clang for external includes
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
    #include <GLFW/glfw3.h>
#pragma clang diagnostic pop

namespace flux::input {
    bool init(InputState* state);
    void deinit(InputState* state);
    void update(InputState* state);

    struct InputState {
        const EngineState* engine;
        bool initialised = false;
    };
}