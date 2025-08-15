#include "engine.hpp"
#include <config.hpp>

#include <renderer/renderer.hpp>
#include <input/input.hpp>

#include "subsystems/log_impl.hpp"
#include "subsystems/ecs_impl.hpp"
#include "subsystems/utility_impl.hpp"
#include "subsystems/math_impl.hpp"
#include "subsystems/allocators_impl.hpp"

i32 main() {
    auto state = new EngineState;
    engine::init(state);

    state->running = true;
    while (state->running)
        engine::update(state);

    engine::deinit(state);
    delete state;
    return EXIT_SUCCESS;
}

static void glfw_error_callback(i32 error, const char* description) {
    log::unbuffered(std::format("GLFW Error {}: {}", error, description), log::level::ERROR);
}

void engine::init(EngineState* state) {

    // init glfw
    log::debug("initialising glfw");
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        log::error("failed to init glfw");
    state->deinitStack.emplace_back([] {
        log::debug("deinitialising glfw");
        glfwTerminate();
    });

    // create window
    log::debug("creating window");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    if (!(state->window = glfwCreateWindow(config::WINDOW_WIDTH, config::WINDOW_HEIGHT, config::APP_NAME.c_str(), nullptr, nullptr)))
        log::error("failed to create window");
    state->deinitStack.emplace_back([state] {
        log::debug("destroying window");
        glfwDestroyWindow(state->window);
    });

    // init renderer
    log::debug("initialising renderer");
    if (!renderer::init(state->renderer = new RendererState { .engine = state }))
        log::error("failed to init renderer");
    state->deinitStack.emplace_back([state] {
        log::debug("deinitialising renderer");
        renderer::deinit(state->renderer);
        delete state->renderer;
    });

    // init input
    log::debug("initialising input");
    if (!input::init(state->input = new InputState { .engine = state }))
        log::error("failed to init input");
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

void engine::update(EngineState* state) {
    glfwPollEvents();

    input::update(state->input);
    renderer::draw(state->renderer);

    if (glfwWindowShouldClose(state->window))
        state->running = false;
}
