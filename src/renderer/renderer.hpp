#pragma once
#include <common.hpp>
#include <config.hpp>
#include <engine/subsystems/math.hpp>
#include <engine/subsystems/utility.hpp>
#include <engine/subsystems/log.hpp>

// silence clang for external includes
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
    #include <vulkan/vulkan.h>
    #include <vulkan/vk_enum_string_helper.h>
    #include <vkb/VkBootstrap.h>
    #include <vma/flux_vk_mem_alloc.h>
    #include <GLFW/glfw3.h>
#pragma clang diagnostic pop

namespace flux::config::renderer {
    static constexpr bool ENABLE_VALIDATION_LAYERS = true;
    static constexpr u32 FRAME_OVERLAP = 2;
    static constexpr u32 MAX_DESCRIPTOR_COUNT = std::numeric_limits<u16>::max(); // 65536
    static constexpr u32 PUSH_CONSTANT_SIZE = 128;
}

namespace flux::renderer {
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

    struct ComputePushConstant {
        u32 textureID;
    };

    static constexpr u32 INVALID_DESCRIPTOR_ID_VALUE = std::numeric_limits<u32>::max();
    enum class UniformBufferId : u32            { INVALID = INVALID_DESCRIPTOR_ID_VALUE };
    enum class StorageBufferId : u32            { INVALID = INVALID_DESCRIPTOR_ID_VALUE };
    enum class CombinedSamplerId : u32          { INVALID = INVALID_DESCRIPTOR_ID_VALUE };
    enum class StorageImageId : u32             { INVALID = INVALID_DESCRIPTOR_ID_VALUE };
    enum class AccelerationStructureId : u32    { INVALID = INVALID_DESCRIPTOR_ID_VALUE };

    enum class Binding : u8 {
        UNIFORM_BUFFER          = 0,
        STORAGE_BUFFER          = 1,
        COMBINED_SAMPLER        = 2,
        STORAGE_IMAGE           = 3,
        ACCELERATION_STRUCTURE  = 4,
    };

    struct AllocatedBuffer {
        VkBuffer buffer = nullptr;
        VmaAllocation allocation = nullptr;
        VmaAllocationInfo info = {};
    };

    struct AllocatedImage {
        VkImage image = nullptr;
        VmaAllocation allocation = nullptr;
        VkExtent3D extent = {};
        VkFormat format = {};
    };

    struct CombinedSampler {
        AllocatedImage image = {};
        VkImageView view = nullptr;
        VkSampler sampler = nullptr;
        CombinedSamplerId id = CombinedSamplerId::INVALID;
        VkDescriptorImageInfo descriptorInfo = {};
    };

    struct StorageImage {
        AllocatedImage image = {};
        VkImageView view = nullptr;
        StorageImageId id = StorageImageId::INVALID;
    };

    struct Vertex {
        glm::vec3 position;
        f32 uv_x;
        glm::vec3 normal;
        f32 uv_y;
        glm::vec4 color;
    };

    struct RendererState {
        const EngineState* engine;
        bool initialised = false;
        f32 renderScale = 1.0f;

        VkInstance instance = nullptr;
        VkDebugUtilsMessengerEXT debugMessenger = nullptr;
        VkPhysicalDevice physicalDevice = nullptr;
        VkDevice device = nullptr;
        VkSurfaceKHR surface = nullptr;
        VmaAllocator allocator = nullptr;

        usize frameNumber = 0;
        FrameData frames[config::renderer::FRAME_OVERLAP] = {};

        struct { VkQueue graphics, compute, transfer; } queue = {};
        struct { u32 graphics, compute, transfer; } queueFamily = {};

        VkSwapchainKHR swapchain = nullptr;
        VkFormat swapchainImageFormat = {};
        VkExtent2D swapchainExtent = {};
        VkExtent2D drawExtent = {};

        VkPipelineLayout globalPipelineLayout = nullptr;
        VkDescriptorSet globalDescriptorSet = {};
        VkDescriptorSetLayout globalDescriptorSetLayout = {};
        VkDescriptorPool globalDescriptorPool = nullptr;

        struct {
            u32 uniformBuffer = 0;
            u32 storageBuffer = 0;
            u32 combinedSampler = 0;
            u32 storageImage = 0;
            u32 accelerationStructure = 0;
        } nextAvailableDecriptorId = {};

        // lists of deleted/newly available descriptor ids
        struct {
            std::vector<UniformBufferId> uniformBuffer = {};
            std::vector<StorageBufferId> storageBuffer = {};
            std::vector<CombinedSamplerId> combinedSampler = {};
            std::vector<StorageImageId> storageImage = {};
            std::vector<AccelerationStructureId> accelerationStructure = {};
        } availableDescriptorId = {};
        
        struct PendingWriteDescriptors {
            union DescriptorInfo { VkDescriptorImageInfo image; VkDescriptorBufferInfo buffer; };
            std::vector<VkWriteDescriptorSet> write = {};
            std::vector<DescriptorInfo> info = {};
        } pendingWriteDescriptors = {};
        
        std::vector<VkImage> swapchainImages = {};
        std::vector<VkImageView> swapchainImageViews = {};

        StorageImage drawImage = {};
        StorageImage depthStencil = {};

        // immediate submit structures
        struct {
            VkFence fence = nullptr;
            VkCommandPool cmdPool = nullptr;
            VkCommandBuffer cmdBuffer = nullptr;
        } immediate = {};

        DeinitStack deinitStack = {};
    };
}