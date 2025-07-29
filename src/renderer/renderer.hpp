#pragma once
#include <common/common.hpp>
#include <common/config.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vkb/VkBootstrap.h>
#include <vk_mem_alloc/vk_mem_alloc.h>

#include <GLFW/glfw3.h>

namespace flux {

    namespace renderer {
        bool init(RendererState* state);
        void deinit(RendererState* state);
        void draw(RendererState* state);

        struct FrameData {
            VkCommandPool cmdPool = nullptr;
            VkCommandBuffer primaryCmdBuffer = nullptr;
            // sync objects
            VkSemaphore swapchainSemaphore = nullptr;
            VkSemaphore renderSemaphore = nullptr;
            VkFence renderFence = nullptr;

            DeinitStack deinitStack = {};
        };

        struct AllocatedImage {
            VkImage image = nullptr;
            VkImageView view = nullptr;
            VmaAllocation allocation = nullptr;
            VkExtent3D extent = {};
            VkFormat fmt = {};
        };
    }

    struct RendererState {
        const EngineState* engine;
        bool initialised = false;
        usize frameNumber = 0;
        renderer::FrameData frames[config::renderer::FRAME_OVERLAP];

        VkInstance instance = nullptr;
        VkDebugUtilsMessengerEXT dbgMessenger = nullptr;
        VkPhysicalDevice physicalDevice = nullptr;
        VkDevice device = nullptr;
        VkSurfaceKHR surface = nullptr;
        VmaAllocator allocator = nullptr;

        // queues
        VkQueue graphicsQueue = nullptr;
        VkQueue computeQueue = nullptr;
        VkQueue transferQueue = nullptr;
        u32 graphicsQueueFamily = {};
        u32 computeQueueFamily = {};
        u32 transferQueueFamily = {};

        // swapchain
        VkSwapchainKHR swapchain = nullptr;
        VkFormat swapchainImageFormat = {};
        VkExtent2D swapchainExtent = {};

        // swapchain data
        std::vector<VkImage> swapchainImages = {};
        std::vector<VkImageView> swapchainImageViews = {};

        // draw data
        renderer::AllocatedImage drawImage = {};
        VkExtent2D drawExtent = {};

        VkDescriptorSetLayout descriptorSetLayout = {};
        VkDescriptorSet descriptorSet = {};

        DeinitStack deinitStack = {};
    };

}