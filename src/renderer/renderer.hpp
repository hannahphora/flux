#pragma once
#include <common/types.hpp>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <string>
#include <vector>
#include <functional>

namespace flux {
//--------------------------------------------------------------------------------------------

struct RendererState {
    const EngineState* engine;
    bool initialised = false;
    usize frameNumber = 0;

    // vulkan handles
    VkInstance vkInstance = nullptr;
    VkDebugUtilsMessengerEXT vkDbgMessenger = nullptr;

    std::vector<std::function<void()>> deletionStack = {};
};

namespace renderer {
    bool init(RendererState* state);
    void deinit(RendererState* state);
};

//--------------------------------------------------------------------------------------------
};