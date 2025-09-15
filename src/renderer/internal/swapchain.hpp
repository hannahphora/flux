#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"
#include "helpers.hpp"
#include "resources.hpp"

namespace flux::renderer::swapchain {

    void destroy(RendererState* state) {
        vkDestroySwapchainKHR(state->device, state->swapchain, nullptr);
        for (auto& imageView : state->swapchainImageViews)
            vkDestroyImageView(state->device, imageView, nullptr);
        state->swapchainImages.clear();
        state->swapchainImageViews.clear();
    }

    void create(RendererState* state, u32 width, u32 height) {
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

    void rebuild(RendererState* state) {
        vkQueueWaitIdle(state->queue.graphics);
        
        // destroy swapchain & draw img
        destroy(state);
        vkDestroyImageView(state->device, state->drawImage.view, nullptr);
        
        state->availableDescriptorId.storageImage.emplace_back(state->drawImage.id);

        auto [w, h] = utility::getWindowSize(state->engine);
        create(state, w, h);
        state->drawImage.image = resources::allocateImage(
            state->allocator,
            {w, h},
            VK_FORMAT_R16G16B16A16_SFLOAT,
            resources::STORAGE_IMAGE_USES
        );
        state->drawImage.view = resources::createImageView(state, state->drawImage.image);
        state->drawImage.id = descriptors::registerStorageImage(state, state->drawImage.view);

        // TODO: update draw img descriptor here
    }

}