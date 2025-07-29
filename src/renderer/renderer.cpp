#define VMA_IMPLEMENTATION
#include "renderer.hpp"

#include <core/engine.hpp>
#include <core/subsystems/log.hpp>
#include <core/subsystems/utility.hpp>

#include "internal/images.hpp"
#include "internal/helpers.hpp"

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
    vkCheck(glfwCreateWindowSurface(state->instance, state->engine->window, nullptr, &state->surface));

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

	state->drawImage.fmt = VK_FORMAT_R16G16B16A16_SFLOAT; // hardcode f32 format
	state->drawImage.extent = { (u32)w, (u32)h, 1U };
	VkImageUsageFlags drawImageUsages = {};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto rImageInfo = vkinit::imageCreateInfo(state->drawImage.fmt, drawImageUsages, state->drawImage.extent);
	VmaAllocationCreateInfo rImageAllocInfo = { // alloc from gpu local memory
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
	vmaCreateImage(state->allocator, &rImageInfo, &rImageAllocInfo, &state->drawImage.image, &state->drawImage.allocation, nullptr);

	auto rImageViewInfo = vkinit::imageViewCreateInfo(state->drawImage.fmt, state->drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCheck(vkCreateImageView(state->device, &rImageViewInfo, nullptr, &state->drawImage.view));

	state->deinitStack.emplace_back([state] {
		vkDestroyImageView(state->device, state->drawImage.view, nullptr);
		vmaDestroyImage(state->allocator, state->drawImage.image, state->drawImage.allocation);
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
    clearValue = { { 0.f, 0.f, flash, 1.f } };

    VkImageSubresourceRange clearRange = vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdClearColorImage(cmd, state->drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
}

void renderer::draw(RendererState* state) {
    // wait on gpu to finish rendering last frame
    vkCheck(vkWaitForFences(state->device, 1, &getCurrentFrame(state).renderFence, true, 1000000000 /*max 1 second timeout*/));
    vkCheck(vkResetFences(state->device, 1, &getCurrentFrame(state).renderFence));

    utility::flushDeinitStack(&getCurrentFrame(state).deinitStack);

    // request image from swapchain
    u32 swapchainImageIndex;
    vkCheck(vkAcquireNextImageKHR(
        state->device, state->swapchain,
        1000000000 /*max 1 second timeout*/,
        getCurrentFrame(state).swapchainSemaphore,
        nullptr, &swapchainImageIndex
    ));

    // reset cmd buffer
    auto cmd = getCurrentFrame(state).primaryCmdBuffer;
    vkCheck(vkResetCommandBuffer(cmd, 0));
    auto cmdBeginInfo = vkinit::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    state->drawExtent.width = state->drawImage.extent.width;
    state->drawExtent.height = state->drawImage.extent.height;

    // records cmds
    vkCheck(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    {
        // transition main draw img to general layout for writing
        vkutil::transitionImage(cmd, state->drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        drawBg(state, cmd);

        // transition draw img and swapchain img to transfer layouts
        vkutil::transitionImage(cmd, state->drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkutil::transitionImage(cmd, state->swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // execute a copy from draw img into the swapchain
        vkutil::copyImageToImage(cmd, state->drawImage.image, state->swapchainImages[swapchainImageIndex], state->drawExtent, state->swapchainExtent);

        // set swapchain img layout to present
        vkutil::transitionImage(cmd, state->swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
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
        .pImageIndices = &swapchainImageIndex,
    };
	vkCheck(vkQueuePresentKHR(state->graphicsQueue, &presentInfo));

	state->frameNumber++;
}
