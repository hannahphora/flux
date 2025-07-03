#include <core/engine.h>
#include <core/log/log.h>

void engine::init(EngineState* engine) {

    // init glfw
    log::unbuffered("engine: initialising glfw");
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    engine->window = glfwCreateWindow(800, 600, "Window", nullptr, nullptr);
    engine->deletionStack.emplace_back([engine]()->bool {
        log::unbuffered("engine: deinitialising glfw");
        glfwDestroyWindow(engine->window);
        glfwTerminate();
        return true;
    });

    // init renderer
    log::unbuffered("engine: initialising renderer");
    engine->renderer = new RendererState { .engine = engine };
    if (!renderer::init(engine->renderer)) {
        log::unbuffered("engine: failed to init renderer", LogLevel::ERROR);
        return;
    }
    engine->deletionStack.emplace_back([engine]()->bool {
        log::unbuffered("engine: deinitialising renderer");
        bool retval;
        if ((retval = !renderer::deinit(engine->renderer)))
            log::unbuffered("engine: failed to deinit renderer", LogLevel::WARNING);
        delete engine->renderer;
        return retval ;
    });

    // init input
    log::unbuffered("engine: initialising input");
    engine->input = new InputState { .engine = engine };
    if (!input::init(engine->input)) {
        log::unbuffered("engine: failed to init input", LogLevel::ERROR);
        return;
    }
    engine->deletionStack.emplace_back([engine]()->bool {
        log::unbuffered("engine: deinitialising input");
        bool retval;
        if ((retval = !input::deinit(engine->input)))
            log::unbuffered("engine: failed to deinit input", LogLevel::WARNING);
        delete engine->input;
        return retval;
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

        if (glfwGetKey(engine->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(engine->window, GLFW_TRUE);
    }
}
