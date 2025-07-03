#include <renderer/renderer.h>
#include <core/log/log.h>

bool renderer::init(RendererState* state) {
    
    vk::ApplicationInfo appInfo = {
        "TestApplication",
        VK_MAKE_API_VERSION(0, 1, 0, 0),
        "No Engine",
        VK_MAKE_API_VERSION(0, 1, 0, 0),
        VK_API_VERSION_1_2
    };

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
