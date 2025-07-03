#include <core/engine.h>
#include <core/log/log.h>

void engine::init(EngineState* engine) {

    // init glfw
    log::unbuffered("initialising glfw");
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    engine->window = glfwCreateWindow(800, 600, "Flux", nullptr, nullptr);
    engine->deletionStack.emplace_back([engine]()->bool {
        log::unbuffered("deinitialising glfw");
        glfwDestroyWindow(engine->window);
        glfwTerminate();
        return true;
    });

    // init renderer
    log::unbuffered("initialising renderer");
    engine->renderer = new RendererState { .engine = engine };
    if (!renderer::init(engine->renderer))
        return;
    engine->deletionStack.emplace_back([engine]()->bool {
        log::unbuffered("deinitialising renderer");
        if (!renderer::deinit(engine->renderer))
            return false;
        delete engine->renderer;
        return true;
    });

    // init input
    log::unbuffered("initialising input");
    engine->input = new InputState { .engine = engine };
    if (!input::init(engine->input))
        return;
    engine->deletionStack.emplace_back([engine]()->bool {
        log::unbuffered("deinitialising input");
        if (!input::deinit(engine->input))
            return false;
        delete engine->input;
        return true;
    });
}

void engine::deinit(EngineState* engine) {
    // run callbacks in deletion stack
    while (!engine->deletionStack.empty()) {
        // TODO: handle errors returned from callbacks here
        engine->deletionStack.back()();
        engine->deletionStack.pop_back();
    }
}

void engine::run(EngineState* engine) {
    while(!glfwWindowShouldClose(engine->window)) {
        glfwPollEvents();
    }
}
