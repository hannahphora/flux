#pragma once
#include <common.h>
#include <config.h>

#include <renderer/renderer.h>
#include <input/input.h>

#include <GLFW/glfw3.h>

namespace flux {
//--------------------------------------------------------------------------------------------

struct EngineState {
    GLFWwindow* window = nullptr;

    RendererState* renderer = nullptr;
    InputState* input = nullptr;

    std::vector<std::function<void()>> deletionStack = {};
};

namespace engine {
    void init(EngineState* state);
    void deinit(EngineState* state);
    void run(EngineState* state);
};

//--------------------------------------------------------------------------------------------
};