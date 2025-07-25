#define VMA_IMPLEMENTATION
#include "renderer.hpp"

#include <core/engine.hpp>
#include <core/subsystems/log/log.hpp>
#include <core/subsystems/utility/utility.hpp>

#include "images.hpp"
#include "helpers.hpp"

using namespace renderer;

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

    // init vma
    VmaAllocatorCreateInfo vmaInfo = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = state->physicalDevice,
        .device = state->device,
        .instance = state->instance,
    };
    vmaCreateAllocator(&vmaInfo, &state->allocator);
    state->deinitStack.emplace_back([state] {
        vmaDestroyAllocator(state->allocator);
    });

    // get queues
    state->graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    state->computeQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
    state->transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
    state->graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    state->computeQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::compute).value();
    state->transferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();
    
    // init swapchain
    i32 w, h; // get window size
    glfwGetWindowSize(state->engine->window, &w, &h);
    createSwapchain(state, w, h);

	state->drawImg.fmt = VK_FORMAT_R16G16B16A16_SFLOAT; // hardcode f32 format
	state->drawImg.extent = { (u32)w, (u32)h, 1U };
	VkImageUsageFlags drawImgUsages = {};
    drawImgUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImgUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImgUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImgUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto rImgInfo = vkinit::imgCreateInfo(state->drawImg.fmt, drawImgUsages, state->drawImg.extent);
	VmaAllocationCreateInfo rImgAllocInfo = { // alloc from gpu local memory
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
	vmaCreateImage(state->allocator, &rImgInfo, &rImgAllocInfo, &state->drawImg.img, &state->drawImg.allocation, nullptr);

	auto rImgViewInfo = vkinit::imgViewCreateInfo(state->drawImg.fmt, state->drawImg.img, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCheck(vkCreateImageView(state->device, &rImgViewInfo, nullptr, &state->drawImg.view));

	state->deinitStack.emplace_back([state] {
		vkDestroyImageView(state->device, state->drawImg.view, nullptr);
		vmaDestroyImage(state->allocator, state->drawImg.img, state->drawImg.allocation);
	});

    // init cmds
    auto cmdPoolInfo = vkinit::cmdPoolCreateInfo(
        state->graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (usize i = 0; i < config::renderer::FRAME_OVERLAP; i++) {
		vkCheck(vkCreateCommandPool(state->device, &cmdPoolInfo, nullptr, &state->frames[i].cmdPool));
		auto cmdAllocInfo = vkinit::cmdBufferAllocInfo(state->frames[i].cmdPool, 1);
		vkCheck(vkAllocateCommandBuffers(state->device, &cmdAllocInfo, &state->frames[i].primaryCmdBuffer));
	}

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
    vkDeviceWaitIdle(state->device);

    // free perframe structures and deinit stack
    for (usize i = 0;  i < config::renderer::FRAME_OVERLAP; i++) {
        vkDestroyCommandPool(state->device, state->frames[i].cmdPool, nullptr);

        vkDestroyFence(state->device, state->frames[i].renderFence, nullptr);
        vkDestroySemaphore(state->device, state->frames[i].renderSemaphore, nullptr);
        vkDestroySemaphore(state->device, state->frames[i].swapchainSemaphore, nullptr);

        utility::flushDeinitStack(&state->frames[i].deinitStack);
    }


    utility::flushDeinitStack(&state->deinitStack);
    state->initialised = false;
}

void drawBg(RendererState* state, VkCommandBuffer cmd) {
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(state->frameNumber / 120.f));
    clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

    VkImageSubresourceRange clearRange = vkinit::imgSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdClearColorImage(cmd, state->drawImg.img, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
}

void renderer::draw(RendererState* state) {
    // wait on gpu to finish rendering last frame
    vkCheck(vkWaitForFences(state->device, 1, &getCurrentFrame(state).renderFence, true, 1000000000 /*max 1 second timeout*/));
    vkCheck(vkResetFences(state->device, 1, &getCurrentFrame(state).renderFence));

    utility::flushDeinitStack(&getCurrentFrame(state).deinitStack);

    // request img from swapchain
    u32 swapchainImgIndex;
    vkCheck(vkAcquireNextImageKHR(
        state->device, state->swapchain,
        1000000000 /*max 1 second timeout*/,
        getCurrentFrame(state).swapchainSemaphore,
        nullptr, &swapchainImgIndex
    ));

    // reset cmd buffer
    auto cmd = getCurrentFrame(state).primaryCmdBuffer;
    vkCheck(vkResetCommandBuffer(cmd, 0));
    auto cmdBeginInfo = vkinit::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    state->drawExtent.width = state->drawImg.extent.width;
    state->drawExtent.height = state->drawImg.extent.height;

    // records cmds
    vkCheck(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    {
        // transition our main draw image into general layout so we can write into it
        // we will overwrite it all so we dont care about what was the older layout
        vkutil::transitionImg(cmd, state->drawImg.img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        drawBg(state, cmd);

        //transition the draw image and the swapchain image into their correct transfer layouts
        vkutil::transitionImg(cmd, state->drawImg.img, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkutil::transitionImg(cmd, state->swapchainImgs[swapchainImgIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // execute a copy from the draw image into the swapchain
        vkutil::copyImgToImg(cmd, state->drawImg.img, state->swapchainImgs[swapchainImgIndex], state->drawExtent, state->swapchainExtent);

        // set swapchain image layout to Present so we can show it on the screen
        vkutil::transitionImg(cmd, state->swapchainImgs[swapchainImgIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
	vkCheck(vkEndCommandBuffer(cmd));

    // submit cmd buffer to queue to execute
    auto cmdInfo = vkinit::cmdBufferSubmitInfo(cmd);	
	auto waitInfo = vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame(state).swapchainSemaphore);
	auto signalInfo = vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame(state).renderSemaphore);
	auto submitInfo = vkinit::submitInfo(&cmdInfo, &signalInfo, &waitInfo);
	vkCheck(vkQueueSubmit2(state->graphicsQueue, 1, &submitInfo, getCurrentFrame(state).renderFence));

    // prepare and present
	VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &getCurrentFrame(state).renderSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &state->swapchain,
        .pImageIndices = &swapchainImgIndex,
    };
	vkCheck(vkQueuePresentKHR(state->graphicsQueue, &presentInfo));

	state->frameNumber++;
}
