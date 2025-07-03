#include <core/engine.h>
#include <log/log.h>

void engine::init(EngineState* state) {

    // init glfw
    log::buffered("engine: initialising glfw");
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    state->window = glfwCreateWindow(800, 600, config::general::APP_NAME.c_str(), nullptr, nullptr);
    state->deletionStack.emplace_back([state]() {
        log::buffered("engine: deinitialising glfw");
        glfwDestroyWindow(state->window);
        glfwTerminate();
    });

    // init renderer
    log::buffered("engine: initialising renderer");
    state->renderer = new RendererState { .engine = state };
    if (!renderer::init(state->renderer)) {
        log::unbuffered("engine: failed to init renderer", LogLevel::ERROR);
        std::exit(EXIT_FAILURE);
    }
    state->deletionStack.emplace_back([state]() {
        log::buffered("engine: deinitialising renderer");
        renderer::deinit(state->renderer);
        delete state->renderer;
    });

    // init input
    log::buffered("engine: initialising input");
    state->input = new InputState { .engine = state };
    if (!input::init(state->input)) {
        log::unbuffered("engine: failed to init input", LogLevel::ERROR);
        std::exit(EXIT_FAILURE);
    }
    state->deletionStack.emplace_back([state]() {
        log::buffered("engine: deinitialising input");
        input::deinit(state->input);
        delete state->input;
    });

    log::flush();
}

void engine::deinit(EngineState* state) {
    // run callbacks in deletion stack
    while (!state->deletionStack.empty()) {
        state->deletionStack.back()();
        state->deletionStack.pop_back();
    }

    log::flush();
}

void engine::run(EngineState* state) {
    while(!glfwWindowShouldClose(state->window)) {
        glfwPollEvents();

        if (glfwGetKey(state->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(state->window, GLFW_TRUE);
    }
}
