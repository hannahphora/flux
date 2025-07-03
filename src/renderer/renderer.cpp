#include <renderer/renderer.h>
#include <log/log.h>

bool renderer::init(RendererState* state) {
    


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
