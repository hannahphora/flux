#include "engine.hpp"
#include <config.hpp>

#include <renderer/renderer.hpp>
#include <input/input.hpp>

#include <subsystems/log.hpp>
#include <subsystems/utility.hpp>
#include <subsystems/math.hpp>

static void glfwErrorCallback(i32 error, const char* description) {
    log::unbuffered(std::format("GLFW Error {}: {}", error, description), log::level::ERROR);
}

void engine::init(EngineState* state) {

    // init glfw
    log::debug("initialising glfw");
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        log::error("failed to init glfw");
        utility::exitWithFailure();
    }
    state->deinitStack.emplace_back([] {
        log::debug("deinitialising glfw");
        glfwTerminate();
    });

    // create window
    log::debug("creating window");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    if (!(state->window = glfwCreateWindow(config::WINDOW_WIDTH, config::WINDOW_HEIGHT, config::APP_NAME.c_str(), nullptr, nullptr))) {
        log::error("failed to create window");
        utility::exitWithFailure();
    }
    state->deinitStack.emplace_back([state] {
        log::debug("destroying window");
        glfwDestroyWindow(state->window);
    });

    // init renderer
    log::debug("initialising renderer");
    if (!renderer::init(state->renderer = new RendererState { .engine = state })) {
        log::error("failed to init renderer");
        utility::exitWithFailure();
    }
    state->deinitStack.emplace_back([state] {
        log::debug("deinitialising renderer");
        renderer::deinit(state->renderer);
        delete state->renderer;
    });

    // init input
    log::debug("initialising input");
    if (!input::init(state->input = new InputState { .engine = state })) {
        log::error("failed to init input");
        utility::exitWithFailure();
    }
    state->deinitStack.emplace_back([state] {
        log::debug("deinitialising input");
        input::deinit(state->input);
        delete state->input;
    });

    state->initialised = true;
}

void engine::deinit(EngineState* state) {
    utility::flushDeinitStack(&state->deinitStack);
    log::flush();
    state->initialised = false;
}

void engine::run(EngineState* state) {
    if (!state->initialised) {
        log::error("failed to run, engine not initialised");
        utility::exitWithFailure();
    }

    while (!glfwWindowShouldClose(state->window)) {
        glfwPollEvents();
    
        input::update(state->input);
        renderer::draw(state->renderer);
    }
}
