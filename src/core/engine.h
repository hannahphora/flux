#pragma once
#include <common.h>
#include <renderer/renderer.h>
#include <input/input.h>

#include <GLFW/glfw3.h>

#include <functional>
#include <vector>

namespace flux {
//--------------------------------------------------------------------------------------------

struct EngineState {
    GLFWwindow* window = nullptr;

    RendererState* renderer = nullptr;
    InputState* input = nullptr;

    std::vector<std::function<bool()>> deletionStack = {};
};

namespace engine {
    void init(EngineState* engine);
    void deinit(EngineState* engine);
    void run(EngineState* engine);
};

//--------------------------------------------------------------------------------------------
};