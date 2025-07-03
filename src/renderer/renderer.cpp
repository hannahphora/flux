#include <renderer/renderer.h>
#include <renderer/internal/initialisers.h>

bool renderer::init(RendererState* state) {
    // create instance
    if (!vki::instance(&state->vkInstance)) {
        log::unbuffered("failed to create vulkan instance", LogLevel::ERROR);
        return false;
    }
    state->deletionStack.emplace_back([state]()->bool {
        vkDestroyInstance(state->vkInstance, nullptr);
        return true;
    });

    state->initialised = true;
    return true;
}

bool renderer::deinit(RendererState* state) {
    // run callbacks in deletion stack
    while (!state->deletionStack.empty()) {
        if (!state->deletionStack.back()())
            return false;
        state->deletionStack.pop_back();
    }

    state->initialised = false;
    return true;
}
