#pragma once
#include <common/common.hpp>
#include <common/config.hpp>
#include <common/math.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vkb/VkBootstrap.h>
#include <vk_mem_alloc/vk_mem_alloc.h>
#include <clay/clay.h>

#include <GLFW/glfw3.h>

namespace flux::renderer {
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
        VkFormat format = {};
    };

    enum class WriteImageID : u32 { Invalid = std::numeric_limits<u32>::max() };
    enum class TextureID : u32 { Invalid = std::numeric_limits<u32>::max() };
    enum class BufferID : u32 { Invalid = std::numeric_limits<u32>::max() };

    static constexpr u32 UniformBinding = 0;
    static constexpr u32 StorageBinding = 1;
    static constexpr u32 TextureBinding = 2;

    namespace ui {
        bool init(UiState* state);
        void deinit(UiState* state);
    }
}

struct flux::UiState {
    const EngineState* engine;
    bool initialised = false;
    DeinitStack deinitStack = {};
};

struct flux::RendererState {
    const EngineState* engine;
    bool initialised = false;

    VkInstance instance = nullptr;
    VkDebugUtilsMessengerEXT dbgMessenger = nullptr;
    VkPhysicalDevice physicalDevice = nullptr;
    VkDevice device = nullptr;
    VmaAllocator allocator = nullptr;

    usize frameNumber = 0;
    renderer::FrameData frames[config::renderer::FRAME_OVERLAP];

    VkQueue graphicsQueue = nullptr;
    VkQueue computeQueue = nullptr;
    VkQueue transferQueue = nullptr;
    u32 graphicsQueueFamily = {};
    u32 computeQueueFamily = {};
    u32 transferQueueFamily = {};

    VkSurfaceKHR surface = nullptr;
    VkSwapchainKHR swapchain = nullptr;
    VkFormat swapchainImageFormat = {};
    VkExtent2D swapchainExtent = {};
    VkExtent2D drawExtent = {};

    // pipelines
    VkPipelineLayout pipelineLayout = nullptr;
    VkPipeline gradientPipeline = nullptr;

    // descriptors
    VkDescriptorSet bindlessDescriptorSet = {};
    VkDescriptorSetLayout bindlessDescriptorSetLayout = {};
    VkDescriptorPool descriptorPool = nullptr;
    
    std::vector<VkImage> swapchainImages = {};
    std::vector<VkImageView> swapchainImageViews = {};
    renderer::AllocatedImage drawImage = {};
    renderer::TextureID drawImageID = {};

    // immediate submit structures
    struct {
        VkFence fence = nullptr;
        VkCommandPool cmdPool = nullptr;
        VkCommandBuffer cmdBuffer = nullptr;
    } immediate = {};

    UiState* ui = nullptr;

    DeinitStack deinitStack = {};
};
