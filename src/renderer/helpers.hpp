#pragma once
#include "renderer.hpp"

namespace flux::renderer {

    inline void vkCheck(VkResult result) {
        if (result) log::unbuffered(
            std::format("Vulkan error: {}", string_VkResult(result)),
            log::level::ERROR
        );
    }

    FrameData& getCurrentFrame(RendererState* state) {
        return state->frames[state->frameNumber % config::renderer::FRAME_OVERLAP];
    };

    inline void destroySwapchain(RendererState* state) {
        vkDestroySwapchainKHR(state->device, state->swapchain, nullptr);
        for (auto& imgView : state->swapchainImgViews)
            vkDestroyImageView(state->device, imgView, nullptr);
    }

    inline void createSwapchain(RendererState* state) {
        auto swapchainBuilder = vkb::SwapchainBuilder{
            state->physicalDevice,
            state->device,
            state->surface
        };
        state->swapchainImgFormat = VK_FORMAT_B8G8R8A8_UNORM;

        i32 w, h;
        glfwGetWindowSize(state->engine->window, &w, &h);
        auto vkbSwapchain = swapchainBuilder
            //.use_default_format_selection()
            .set_desired_format(VkSurfaceFormatKHR{
                .format = state->swapchainImgFormat,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            })
            // use vsync present mode
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(w, h)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();
        
        state->swapchainExtent = vkbSwapchain.extent;
        state->swapchain = vkbSwapchain.swapchain;
        state->swapchainImgs = vkbSwapchain.get_images().value();
        state->swapchainImgViews = vkbSwapchain.get_image_views().value();

        state->deinitStack.emplace_back([state] {
            destroySwapchain(state);
        });
    }

    namespace vkinit {

        VkCommandPoolCreateInfo cmdPoolCreateInfo(u32 queueFamilyIndex,
            VkCommandPoolCreateFlags flags = 0) {
            return {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = flags,
                .queueFamilyIndex = queueFamilyIndex,
            };
        }

        VkCommandBufferAllocateInfo cmdBufferAllocInfo(
            VkCommandPool pool, u32 count = 1) {
            return {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = count,
            };
        }

        VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0) {
            return {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = flags,
            };
        }

        VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0) {
            return {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .flags = flags,
            }
        }

    }
}
