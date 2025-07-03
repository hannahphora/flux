#pragma once
#include <common.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <vector>

namespace flux {
//--------------------------------------------------------------------------------------------

struct RendererState {
    const EngineState* engine;
    bool initialised = false;
    usize frameNumber = 0;

    // vulkan handles
    VkInstance vkInstance = nullptr;

    std::vector<std::function<bool()>> deletionStack = {};
};

namespace renderer {
    bool init(RendererState* state);
    bool deinit(RendererState* state);
};

//--------------------------------------------------------------------------------------------
};