#pragma once
#include <common.hpp>

// silence clang for external includes
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
    #include <GLFW/glfw3.h>
#pragma clang diagnostic pop

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
        AudioState* audio = nullptr;
        PhysicsState* physics = nullptr;
        NetworkingState* networking = nullptr;
        AiState* ai = nullptr;

        DeinitStack deinitStack = {};
    };
}