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
    
    // app info
    auto appInfo = vk::ApplicationInfo{
        config::general::APP_NAME.c_str(), 1,       // app name/version
        config::general::ENGINE_NAME.c_str(), 1,    // engine name/version
        VK_API_VERSION_1_3
    };

    // extensions
    auto glfwExtCount = 0u;
    auto glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    auto exts = std::vector<const char*>(glfwExts, glfwExts + glfwExtCount);
    exts.emplace_back("VK_EXT_debug_utils");
    exts.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    // layers
    auto layers = std::vector<const char*>{ "VK_LAYER_KHRONOS_validation" };

    // instance
    state->instance = vk::createInstance(vk::InstanceCreateInfo{
        vk::InstanceCreateFlags{ VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR },
        &appInfo,
        (u32)layers.size(), layers.data(),  // layers size/data
        (u32)exts.size(), exts.data()       // extensions size/data
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
