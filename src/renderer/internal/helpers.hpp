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

    void createDrawImage(RendererState* state, u32 width, u32 height) {
        state->drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT; // hardcode f32 format
        state->drawImage.extent = { width, height, 1U };
        VkImageUsageFlags drawImageUsages = {};
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto imageInfo = vkinit::imageCreateInfo(state->drawImage.format, drawImageUsages, state->drawImage.extent);
        VmaAllocationCreateInfo imageAllocInfo = { // alloc from gpu local memory
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = (VkMemoryPropertyFlags)VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };
        vmaCreateImage(state->allocator, &imageInfo, &imageAllocInfo, &state->drawImage.image, &state->drawImage.allocation, nullptr);

        auto viewInfo = vkinit::imageViewCreateInfo(state->drawImage.format, state->drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCheck(vkCreateImageView(state->device, &viewInfo, nullptr, &state->drawImage.view));
    }

    void rebuildSwapchain(RendererState* state) {
        vkQueueWaitIdle(state->graphicsQueue);
        
        // destroy swapchain & draw img
        destroySwapchain(state);
        vkDestroyImageView(state->device, state->drawImage.view, nullptr);
        vmaDestroyImage(state->allocator, state->drawImage.image, state->drawImage.allocation);

        auto [w, h] = utility::getWindowSize(state->engine);
        createSwapchain(state, w, h);
        createDrawImage(state, w, h);

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
        usize fileSize = (size_t)file.tellg();
        std::vector<u32> buffer(fileSize / sizeof(u32));
        file.seekg(0);
        file.read((char*)buffer.data(), fileSize);
        file.close();
        VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = buffer.size() * sizeof(u32), // size in bytes
            .pCode = buffer.data(),
        };
        return vkCreateShaderModule(device, &createInfo, nullptr, out) == VK_SUCCESS;
    }

    void initDrawImageDescriptor(RendererState* state) {
        VkDescriptorImageInfo imageInfo = {
            .imageView = state->drawImage.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        VkWriteDescriptorSet drawImageWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = state->bindlessDescriptorSet,
            .dstBinding = (u32)STORAGE_IMAGE_BINDING,
            .dstArrayElement = (u32)state->drawImageID,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &imageInfo,
        };
        vkUpdateDescriptorSets(state->device, 1, &drawImageWrite, 0, nullptr);
    }

}
