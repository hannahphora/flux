#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"

namespace flux::renderer {

    void vkCheck(VkResult result) {
        if (result) log::unbuffered(std::format("Vulkan error: {}", string_VkResult(result)), log::level::ERROR);
    }

    FrameData& getCurrentFrame(RendererState* state) {
        return state->frames[state->frameNumber % config::renderer::FRAME_OVERLAP];
    };

}

namespace flux::renderer::vkutil {

    void destroySwapchain(RendererState* state) {
        vkDestroySwapchainKHR(state->device, state->swapchain, nullptr);
        for (auto& imageView : state->swapchainImageViews)
            vkDestroyImageView(state->device, imageView, nullptr);
        state->swapchainImages.clear();
        state->swapchainImageViews.clear();
    }

    void createSwapchain(RendererState* state, u32 width, u32 height) {
        auto swapchainBuilder = vkb::SwapchainBuilder{ state->physicalDevice, state->device, state->surface };
        state->swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        auto vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // vsync
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();
        
        state->swapchain = vkbSwapchain.swapchain;
        state->swapchainExtent = vkbSwapchain.extent;
        state->swapchainImageFormat = vkbSwapchain.image_format;
        state->swapchainImages = vkbSwapchain.get_images().value();
        state->swapchainImageViews = vkbSwapchain.get_image_views().value();
    }

    void createImage(RendererState* state, u32 width, u32 height, Image* out) {
        out->extent = { width, height, 1U },
        out->format = VK_FORMAT_R16G16B16A16_SFLOAT, // hardcode f32 format
        VkImageUsageFlags usages = {};
        usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usages |= VK_IMAGE_USAGE_STORAGE_BIT;
        usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        auto imageInfo = vkinit::imageCreateInfo(out->format, usages, out->extent);
        VmaAllocationCreateInfo imageAllocInfo = { // alloc from gpu local memory
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = (VkMemoryPropertyFlags)VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };
        vmaCreateImage(state->allocator, &imageInfo, &imageAllocInfo, &out->image, &out->allocation, nullptr);
        auto viewInfo = vkinit::imageViewCreateInfo(out->format, out->image, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCheck(vkCreateImageView(state->device, &viewInfo, nullptr, &out->view));
    }

    void rebuildSwapchain(RendererState* state) {
        vkQueueWaitIdle(state->graphicsQueue);
        
        // destroy swapchain & draw img
        destroySwapchain(state);
        vkDestroyImageView(state->device, state->drawImage.view, nullptr);
        vmaDestroyImage(state->allocator, state->drawImage.image, state->drawImage.allocation);
        state->availableImageIds.emplace_back(state->drawImage.id);

        auto [w, h] = utility::getWindowSize(state->engine);
        createSwapchain(state, w, h);
        state->drawImage = createImage(state, w, h);

        // TODO: update draw img descriptor here
    }

    void immediateSubmit(RendererState* state, std::function<void(VkCommandBuffer cmd)>&& fn) {
        vkCheck(vkResetFences(state->device, 1, &state->immediate.fence));
        vkCheck(vkResetCommandBuffer(state->immediate.cmdBuffer, 0));

        VkCommandBuffer cmd = state->immediate.cmdBuffer;
        auto cmdBeginInfo = vkinit::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        vkCheck(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
        fn(cmd);
        vkCheck(vkEndCommandBuffer(cmd));

        // NOTE: render fence will now block until graphics cmds finish execution
        auto cmdInfo = vkinit::cmdBufferSubmitInfo(cmd);
        auto submit = vkinit::submitInfo(&cmdInfo, nullptr, nullptr);
        vkCheck(vkQueueSubmit2(state->graphicsQueue, 1, &submit, state->immediate.fence));
        vkCheck(vkWaitForFences(state->device, 1, &state->immediate.fence, true, 9999999999));
    }

    bool loadShaderModule(const char* path, VkDevice device, VkShaderModule* out) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) return false;
        usize size = (usize)file.tellg();
        std::vector<u32> buffer(size / sizeof(u32));
        file.seekg(0);
        file.read((char*)buffer.data(), size);
        file.close();
        VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = buffer.size() * sizeof(u32), // size in bytes
            .pCode = buffer.data(),
        };
        return vkCreateShaderModule(device, &createInfo, nullptr, out) == VK_SUCCESS;
    }

    void initPipelineLayout(RendererState* state) {
        VkPushConstantRange pushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset = 0U,
            .size = 128U,
        };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &state->globalDescriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
        };
        vkCreatePipelineLayout(state->device, &pipelineLayoutInfo, nullptr, &state->globalPipelineLayout);
        state->deinitStack.emplace_back([state] {
            vkDestroyPipelineLayout(state->device, state->globalPipelineLayout, nullptr);
        });
    }

}
