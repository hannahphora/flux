#pragma once
#include <common/common.hpp>
#include <common/config.hpp>

#include <GLFW/glfw3.h>

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
    extern "C" __declspec(dllexport) void init(EngineState* state);
    extern "C" __declspec(dllexport) void deinit(EngineState* state);
    extern "C" __declspec(dllexport) void update(EngineState* state);
};

//--------------------------------------------------------------------------------------------
};