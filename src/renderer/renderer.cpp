#include <renderer/renderer.h>
#include <log/log.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    // TODO: replace this with log::unbuffered
    fprintf(stdout, "validation layer: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

bool renderer::init(RendererState* state) {
    
    auto appInfo = vk::ApplicationInfo {
        
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
