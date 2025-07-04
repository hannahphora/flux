#pragma once
#include <common/types.hpp>
#include <common/config.hpp>

#include <GLFW/glfw3.h>

#include <cstdlib>
#include <string>
#include <vector>
#include <functional>

namespace flux {
//--------------------------------------------------------------------------------------------

struct EngineState {
    bool running = false;
    GLFWwindow* window = nullptr;
    RendererState* renderer = nullptr;
    InputState* input = nullptr;
    std::vector<std::function<void()>> deletionStack = {};
};

namespace engine {
    extern "C" void init(EngineState* state);
    extern "C" void deinit(EngineState* state);
    extern "C" void process_frame(EngineState* state);
};

//--------------------------------------------------------------------------------------------
};