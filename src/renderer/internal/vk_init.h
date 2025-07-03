#pragma once
#include <renderer/renderer.h>

namespace vk {
//--------------------------------------------------------------------------------------------

bool createInstance(VkInstance* instance) {
    auto appInfo = VkApplicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "TestApplication",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Flux",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    u32 glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    
    auto createInfo = VkInstanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .enabledExtensionCount = glfwExtCount,
        .ppEnabledExtensionNames = glfwExts,
    };

    return vkCreateInstance(&createInfo, nullptr, instance) == VK_SUCCESS;
}

//--------------------------------------------------------------------------------------------
};