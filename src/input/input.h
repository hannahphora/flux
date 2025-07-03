#pragma once
#include <common.h>

#include <GLFW/glfw3.h>

namespace flux {
//--------------------------------------------------------------------------------------------

struct InputConfig {

};

struct InputState {
    const EngineState* engine;
    bool initialised = false;
};

namespace input {
    bool init(InputState* state);
    bool deinit(InputState* state);

    // TEMPORARY
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
};

//--------------------------------------------------------------------------------------------
};