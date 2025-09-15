#pragma once
#include <common.hpp>
#include <config.hpp>
#include <subsystems/math.hpp>
#include <subsystems/utility.hpp>
#include <subsystems/log.hpp>

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

    enum class UniformBufferId : u32            { INVALID = 16 };
    enum class CombinedSamplerId : u32          { INVALID = std::numeric_limits<u16>::max() };
    enum class StorageImageId : u32             { INVALID = std::numeric_limits<u16>::max() };
    enum class AccelerationStructureId : u32    { INVALID = std::numeric_limits<u16>::max() };

    enum class Binding : u8 {
        UNIFORM_BUFFER          = 0,
        COMBINED_SAMPLER        = 1,
        STORAGE_IMAGE           = 2,
        ACCELERATION_STRUCTURE  = 3,
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

    // holds the resources needed for a mesh
    struct GPUMeshBuffers {
        AllocatedBuffer indexBuffer;
        AllocatedBuffer vertexBuffer;
        VkDeviceAddress vertexBufferAddress;
    };

    // push constants for mesh object draws
    struct GPUDrawPushConstants {
        glm::mat4 worldMatrix;
        VkDeviceAddress vertexBuffer;
    };

    struct GeoSurface {
        u32 startIndex;
        u32 count;
    };

    struct MeshAsset {
        std::string name;
        std::vector<GeoSurface> surfaces;
        GPUMeshBuffers meshBuffers;
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

        VkDescriptorSet globalDescriptorSet = {};
        VkDescriptorSetLayout globalDescriptorSetLayout = {};
        VkDescriptorPool globalDescriptorPool = nullptr;
        VkDescriptorPool imguiDescriptorPool = nullptr;

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

        VkPipelineLayout globalPipelineLayout = nullptr;
        VkPipeline pipeline;

        struct {
            VkFence fence = nullptr;
            VkCommandPool cmdPool = nullptr;
            VkCommandBuffer cmdBuffer = nullptr;
        } immediateSubmit = {};

        DeinitStack deinitStack = {};
    };
}