#pragma once
#include <common/common.hpp>

#include <GLFW/glfw3.h>

namespace flux::engine {
    extern "C" __declspec(dllexport) void init(EngineState* state);
    extern "C" __declspec(dllexport) void deinit(EngineState* state);
    extern "C" __declspec(dllexport) void update(EngineState* state);

    struct EngineState {
        bool initialised = false;
        bool running = false;
        GLFWwindow* window = nullptr;

        RendererState* renderer = nullptr;
        InputState* input = nullptr;

        DeinitStack deinitStack = {};
    };
}