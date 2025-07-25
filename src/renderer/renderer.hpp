#pragma once
#include <common/common.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vkb/VkBootstrap.h>
#include <GLFW/glfw3.h>

namespace flux {
//--------------------------------------------------------------------------------------------

struct RendererState {
    const EngineState* engine;
    bool initialised = false;
    usize frameNumber = 0;

    VkInstance vkInstance = nullptr;
    VkDebugUtilsMessengerEXT vkDbgMessenger = nullptr;

    std::vector<std::function<void()>> deletionStack = {};
};

namespace renderer {
    bool init(RendererState* state);
    void deinit(RendererState* state);

    void vkCheck(VkResult result);
};

//--------------------------------------------------------------------------------------------
};