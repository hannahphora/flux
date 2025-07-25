#include <renderer/renderer.hpp>
#include <core/engine.hpp>
#include <core/subsystems/log/log.hpp>

#include "images.hpp"
#include "helpers.hpp"

bool renderer::init(RendererState* state) {
    // create instance
    vkb::InstanceBuilder instBuilder;
    auto vkbInst = instBuilder
        .set_app_name(config::APP_NAME.c_str())
        .request_validation_layers(config::renderer::ENABLE_VALIDATION_LAYERS)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build()
        .value();
    state->instance = vkbInst.instance;
    state->dbgMessenger = vkbInst.debug_messenger;
    state->deinitStack.emplace_back([state] {
        vkb::destroy_debug_utils_messenger(state->instance, state->dbgMessenger);
		vkDestroyInstance(state->instance, nullptr);
    });

    // create surface
    vkCheck(glfwCreateWindowSurface(
        state->instance,
        state->engine->window,
        nullptr,
        &state->surface
    ));

    // query physical device and create device
    auto physDevSelector = vkb::PhysicalDeviceSelector{ vkbInst };
    auto vkbPhysDev = physDevSelector
        .set_minimum_version(1, 3)
        .set_required_features_13({
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .synchronization2 = true,
            .dynamicRendering = true,
        })
        .set_required_features_12({
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .descriptorIndexing = true,
            .bufferDeviceAddress = true,
        })
        .set_surface(state->surface)
        .select()
        .value();
    auto devBuilder = vkb::DeviceBuilder{ vkbPhysDev };
    auto vkbDevice = devBuilder.build().value();
    state->device = vkbDevice.device;
    state->physicalDevice = vkbPhysDev.physical_device;

    // NOTE: surface must be destroyed b4 device
    state->deinitStack.emplace_back([state] {
        vkDestroySurfaceKHR(state->instance, state->surface, nullptr);
        vkDestroyDevice(state->device, nullptr);
    });

    // get queues
    state->graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    state->computeQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
    state->transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
    state->graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    state->computeQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::compute).value();
    state->transferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();
    
    // init swapchain
    createSwapchain(state);

    // init cmds
    auto cmdPoolInfo = vkinit::cmdPoolCreateInfo(
        state->graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (usize i = 0; i < config::renderer::FRAME_OVERLAP; i++) {
		vkCheck(vkCreateCommandPool(state->device, &cmdPoolInfo, nullptr, &state->frames[i].cmdPool));
		auto cmdAllocInfo = vkinit::cmdBufferAllocInfo(state->frames[i].cmdPool, 1);
		vkCheck(vkAllocateCommandBuffers(state->device, &cmdAllocInfo, &state->frames[i].primaryCmdBuffer));
	}
    state->deinitStack.emplace_back([state] {
        vkDeviceWaitIdle(state->device);
        for (usize i = 0;  i < config::renderer::FRAME_OVERLAP; i++)
            vkDestroyCommandPool(state->device, state->frames[i].cmdPool, nullptr);
    });

    // init sync structures
    auto fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    auto semCreateInfo = vkinit::semaphoreCreateInfo();
    for (usize i = 0;  i < config::renderer::FRAME_OVERLAP; i++) {
        vkCheck(vkCreateFence(state->device, &fenceCreateInfo, nullptr, &state->frames[i].renderFence));
        vkCheck(vkCreateSemaphore(state->device, &semCreateInfo, nullptr, &state->frames[i].swapchainSemaphore));
        vkCheck(vkCreateSemaphore(state->device, &semCreateInfo, nullptr, &state->frames[i].renderSemaphore));
    }

    state->initialised = true;
    return true;
}

void renderer::deinit(RendererState* state) {
    // run callbacks in deletion stack
    while (!state->deinitStack.empty()) {
        state->deinitStack.back()();
        state->deinitStack.pop_back();
    }

    state->initialised = false;
}

void renderer::draw(RendererState* state) {
    // wait on gpu to finish rendering last frame
    vkCheck(vkWaitForFences(state->device, 1, &getCurrentFrame(state).renderFence, true, 1000000000 /*max 1 second timeout*/));
    vkCheck(vkResetFences(state->device, 1, &getCurrentFrame(state).renderFence));

    // request img from swapchain
    u32 swapchainImgIndex;
    vkCheck(vkAcquireNextImageKHR(
        state->device, state->swapchain,
        1000000000 /*max 1 second timeout*/,
        getCurrentFrame(state).swapchainSemaphore,
        nullptr, &swapchainImgIndex
    ));

    // reset cmd buffer and records cmds
    auto cmd = getCurrentFrame(state).primaryCmdBuffer;
    vkCheck(vkResetCommandBuffer(cmd, 0));
    auto cmdBeginInfo = vkinit::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkCheck(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    
}
