#pragma once
#include <common.h>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <functional>
#include <vector>

namespace flux {
//--------------------------------------------------------------------------------------------

struct RendererConfig {
    static constexpr bool enableValidationLayers = true;
};

struct RendererState {
    const EngineState* engine;
    bool initialised = false;
    usize frameNumber = 0;

    // vulkan handles
    VkInstance vkInstance = nullptr;
    VkDebugUtilsMessengerEXT vkDbgMessenger = nullptr;

    std::vector<std::function<bool()>> deletionStack = {};
};

namespace renderer {
    bool init(RendererState* state);
    bool deinit(RendererState* state);
};

//--------------------------------------------------------------------------------------------
};