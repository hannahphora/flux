#pragma once
#include <common/common.hpp>
#include <common/config.hpp>
#include <common/math.hpp>

// silence clang for external includes
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
    #include <vulkan/vulkan.h>
    #include <vulkan/vk_enum_string_helper.h>
    #include <vkb/VkBootstrap.h>
    #include <vk_mem_alloc/vk_mem_alloc.h>
    #include <GLFW/glfw3.h>
    #include <imgui/imgui.h>
    #include <imgui/imgui_internal.h>
    #include <imgui/backends/imgui_impl_glfw.h>
    #include <imgui/backends/imgui_impl_vulkan.h>
#pragma clang diagnostic pop

namespace flux::config::renderer {
    static constexpr bool ENABLE_VALIDATION_LAYERS = true;
    static constexpr u32 FRAME_OVERLAP = 2;
    static constexpr usize MAX_DESCRIPTOR_COUNT = std::numeric_limits<u16>::max(); // 65536
}

namespace flux::renderer {
    namespace ui { extern void loadImguiContext(RendererState* state); }
    
    bool init(RendererState* state);
    void deinit(RendererState* state);
    void draw(RendererState* state);

    struct FrameData {
        VkCommandPool cmdPool = nullptr;
        VkCommandBuffer primaryCmdBuffer = nullptr;
        VkSemaphore swapchainSemaphore, renderSemaphore;
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

    struct ComputePushConstant {
        u32 textureID;
    };

    enum class UniformId : u32 { Invalid = std::numeric_limits<u32>::max() };
    enum class BufferId : u32 { Invalid = std::numeric_limits<u32>::max() };
    enum class TextureId : u32 { Invalid = std::numeric_limits<u32>::max() };
    enum class ImageId : u32 { Invalid = std::numeric_limits<u32>::max() };
    enum class AccelerationStructureId : u32 { Invalid = std::numeric_limits<u32>::max() };

    enum class BINDING : u8 {
        UNIFORM = 0,
        BUFFER = 1,
        TEXTURE = 2,
        IMAGE = 3,
        ACCELERATION_STRUCTURE = 4,
    };

    struct RendererState {
        const EngineState* engine;
        bool initialised = false;

        VkInstance instance = nullptr;
        VkDebugUtilsMessengerEXT debugMessenger = nullptr;
        VkPhysicalDevice physicalDevice = nullptr;
        VkDevice device = nullptr;
        VmaAllocator allocator = nullptr;

        usize frameNumber = 0;
        FrameData frames[config::renderer::FRAME_OVERLAP] = {};

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
        VkPipelineLayout globalPipelineLayout = nullptr;
        VkPipeline gradientPipeline = nullptr;

        // descriptors
        VkDescriptorSet globalDescriptorSet = {};
        VkDescriptorSetLayout globalDescriptorSetLayout = {};
        VkDescriptorPool globalDescriptorPool = nullptr;
        
        std::vector<VkImage> swapchainImages = {};
        std::vector<VkImageView> swapchainImageViews = {};
        AllocatedImage drawImage = {};
        ImageId drawImageID = {};

        // imgui
        ImGuiContext* imguiContext = nullptr;
        void* imguiVulkanData = nullptr;
        VkDescriptorPool imguiDescriptorPool = nullptr;

        // immediate submit structures
        struct {
            VkFence fence = nullptr;
            VkCommandPool cmdPool = nullptr;
            VkCommandBuffer cmdBuffer = nullptr;
        } immediate = {};

        DeinitStack deinitStack = {};
    };
}