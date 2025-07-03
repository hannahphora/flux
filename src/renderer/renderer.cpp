#include <renderer/renderer.h>
#include <log/log.h>

bool renderer::init(RendererState* state) {
    
    state->initialised = true;
    return true;
}

void renderer::deinit(RendererState* state) {
    // run callbacks in deletion stack
    while (!state->deletionStack.empty()) {
        state->deletionStack.back()();
        state->deletionStack.pop_back();
    }

    state->initialised = false;
}
