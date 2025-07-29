#include "engine.hpp"
#include <common/config.hpp>

#include <renderer/renderer.hpp>
#include <input/input.hpp>

#include "subsystems/log.hpp"
#include "internal/log_impl.hpp"

#include "subsystems/utility.hpp"
#include "internal/utility_impl.hpp"

void engine::init(EngineState* state) {

    // init glfw
    log::buffered("initialising glfw");
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    state->deinitStack.emplace_back([state] {
        log::buffered("deinitialising glfw");
        glfwTerminate();
    });

    // create window
    log::buffered("creating window");
    if (!(state->window = glfwCreateWindow(
        config::WINDOW_WIDTH, config::WINDOW_HEIGHT,
        config::APP_NAME.c_str(),
        nullptr, nullptr
    ))) {
        log::unbuffered("failed to create window", log::level::ERROR);
    }
    state->deinitStack.emplace_back([state] {
        log::buffered("destroying window");
        glfwDestroyWindow(state->window);
    });

    // init renderer
    log::buffered("initialising renderer");
    if (!renderer::init(state->renderer = new RendererState { .engine = state }))
        log::unbuffered("failed to init renderer", log::level::ERROR);
    state->deinitStack.emplace_back([state] {
        log::buffered("deinitialising renderer");
        renderer::deinit(state->renderer);
        delete state->renderer;
    });

    // init input
    log::buffered("initialising input");
    if (!input::init(state->input = new InputState { .engine = state }))
        log::unbuffered("failed to init input", log::level::ERROR);
    state->deinitStack.emplace_back([state] {
        log::buffered("deinitialising input");
        input::deinit(state->input);
        delete state->input;
    });

    log::flush();
}

void engine::deinit(EngineState* state) {
    utility::flushDeinitStack(&state->deinitStack);
    log::flush();
}

void engine::update(EngineState* state) {
    glfwPollEvents();

    input::update(state->input);
    renderer::draw(state->renderer);

    if (glfwWindowShouldClose(state->window))
        state->running = false;
}
