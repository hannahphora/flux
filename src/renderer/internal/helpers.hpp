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

    void destroySwapchain(RendererState* state) {
        vkDestroySwapchainKHR(state->device, state->swapchain, nullptr);
        for (auto& imageView : state->swapchainImageViews)
            vkDestroyImageView(state->device, imageView, nullptr);
    }

    void createSwapchain(RendererState* state, u32 width, u32 height) {
        auto swapchainBuilder = vkb::SwapchainBuilder{
            state->physicalDevice,
            state->device,
            state->surface
        };
        state->swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        auto vkbSwapchain = swapchainBuilder
            //.use_default_format_selection()
            .set_desired_format(VkSurfaceFormatKHR{
                .format = state->swapchainImageFormat,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            })
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // vsync
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();
        
        state->swapchainExtent = vkbSwapchain.extent;
        state->swapchain = vkbSwapchain.swapchain;
        state->swapchainImages = vkbSwapchain.get_images().value();
        state->swapchainImageViews = vkbSwapchain.get_image_views().value();

        state->deinitStack.emplace_back([state] {
            destroySwapchain(state);
        });
    }

    void rebuildSwapchain(RendererState* state) {
    }

    void immediateSubmit(RendererState* state, std::function<void(VkCommandBuffer cmd)>&& fn) {
        vkCheck(vkResetFences(state->device, 1, &state->immediate.fence));
        vkCheck(vkResetCommandBuffer(state->immediate.cmdBuffer, 0));

        VkCommandBuffer cmd = state->immediate.cmdBuffer;
        auto cmdBeginInfo = vkinit::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        vkCheck(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
        fn(cmd);
        vkCheck(vkEndCommandBuffer(cmd));

        // submit command buffer to the queue and execute it
        //  render fence will now block until the graphic cmds finish execution
        auto cmdInfo = vkinit::cmdBufferSubmitInfo(cmd);
        auto submit = vkinit::submitInfo(&cmdInfo, nullptr, nullptr);
        vkCheck(vkQueueSubmit2(state->graphicsQueue, 1, &submit, state->immediate.fence));
        vkCheck(vkWaitForFences(state->device, 1, &state->immediate.fence, true, 9999999999));
    }
}
