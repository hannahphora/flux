#pragma once
#include <common.h>
#include <config.h>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace flux {
//--------------------------------------------------------------------------------------------

struct RendererState {
    const EngineState* engine;
    bool initialised = false;
    usize frameNumber = 0;

    // vulkan handles
    vk::Instance instance = nullptr;
    vk::DebugUtilsMessengerEXT dbgMessenger = nullptr;

    std::vector<std::function<bool()>> deletionStack = {};
};

namespace renderer {
    bool init(RendererState* state);
    bool deinit(RendererState* state);
};

//--------------------------------------------------------------------------------------------
};