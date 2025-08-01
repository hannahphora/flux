#pragma once
#include <common/common.hpp>
#include <common/config.hpp>
#include <common/math.hpp>
#include <common/utility.hpp>
#include <common/log.hpp>

// silence clang for external includes
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
    #include <vulkan/vulkan.h>
    #include <vulkan/vk_enum_string_helper.h>
    #include <vkb/VkBootstrap.h>
    #include <vk_mem_alloc/vk_mem_alloc.h>
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

    struct Shader {
        std::string name = "shader";
        VkShaderStageFlagBits stage = {};
        VkShaderStageFlags nextStage = {};
        VkShaderEXT shader = nullptr;
        VkShaderCreateInfoEXT createInfo = {};
        std::vector<u32> spirv = {};
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

        // nk ui
        struct {
            struct nk_context {}; struct nk_colorf {}; struct nk_image {}; // TMP
            struct nk_context* ctx  = nullptr;
            struct nk_colorf bg = {};
            struct nk_image img = {};
            VkSemaphore semaphore = nullptr;
        } nk = {};

        // immediate submit structures
        struct {
            VkFence fence = nullptr;
            VkCommandPool cmdPool = nullptr;
            VkCommandBuffer cmdBuffer = nullptr;
        } immediate = {};

        DeinitStack deinitStack = {};

        PFN_vkCreateShadersEXT vkCreateShadersEXT{ VK_NULL_HANDLE };
        PFN_vkDestroyShaderEXT vkDestroyShaderEXT{ VK_NULL_HANDLE };
        PFN_vkCmdBindShadersEXT vkCmdBindShadersEXT{ VK_NULL_HANDLE };
        PFN_vkGetShaderBinaryDataEXT vkGetShaderBinaryDataEXT{ VK_NULL_HANDLE };

        // VK_EXT_shader_objects requires render passes to be dynamic
        PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR{ VK_NULL_HANDLE };
        PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR{ VK_NULL_HANDLE };

        // With VK_EXT_shader_object pipeline state must be set at command buffer creation using these functions
        PFN_vkCmdSetAlphaToCoverageEnableEXT vkCmdSetAlphaToCoverageEnableEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetColorBlendEnableEXT vkCmdSetColorBlendEnableEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetColorWriteMaskEXT vkCmdSetColorWriteMaskEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetCullModeEXT vkCmdSetCullModeEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetDepthBiasEnableEXT vkCmdSetDepthBiasEnableEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetDepthCompareOpEXT vkCmdSetDepthCompareOpEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetDepthTestEnableEXT vkCmdSetDepthTestEnableEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetDepthWriteEnableEXT vkCmdSetDepthWriteEnableEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetFrontFaceEXT vkCmdSetFrontFaceEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetPolygonModeEXT vkCmdSetPolygonModeEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetPrimitiveRestartEnableEXT vkCmdSetPrimitiveRestartEnableEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetPrimitiveTopologyEXT vkCmdSetPrimitiveTopologyEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetRasterizationSamplesEXT vkCmdSetRasterizationSamplesEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetRasterizerDiscardEnableEXT vkCmdSetRasterizerDiscardEnableEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetSampleMaskEXT vkCmdSetSampleMaskEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetScissorWithCountEXT vkCmdSetScissorWithCountEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetStencilTestEnableEXT vkCmdSetStencilTestEnableEXT{ VK_NULL_HANDLE };
        PFN_vkCmdSetViewportWithCountEXT vkCmdSetViewportWithCountEXT{ VK_NULL_HANDLE };

        // VK_EXT_vertex_input_dynamic_state
        PFN_vkCmdSetVertexInputEXT vkCmdSetVertexInputEXT{ VK_NULL_HANDLE };
    };
}