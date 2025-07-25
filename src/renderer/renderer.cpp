#include <renderer/renderer.hpp>
#include <core/subsystems/log/log.hpp>

using namespace renderer::internal;

void vkCheck(VkResult result) {
    if (result) {
        log::unbuffered(std::format("Vulkan error: {}", string_VkResult(result)), log::level::ERROR);
        std::abort();
    }
}

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
