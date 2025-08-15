#pragma once
#include <common.hpp>

#include <GLFW/glfw3.h>

namespace flux::engine {
    void init(EngineState* state);
    void deinit(EngineState* state);
    void update(EngineState* state);

    struct EngineState {
        bool initialised = false;
        bool running = false;
        GLFWwindow* window = nullptr;

        RendererState* renderer = nullptr;
        InputState* input = nullptr;
        AudioState* input = nullptr;
        PhysicsState* input = nullptr;

        DeinitStack deinitStack = {};
    };
}