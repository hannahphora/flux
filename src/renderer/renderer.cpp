#define VMA_IMPLEMENTATION
#include "renderer.hpp"

#include <core/engine.hpp>

#include "internal/images.hpp"
#include "internal/helpers.hpp"
#include "internal/descriptors.hpp"
#include "internal/ui.hpp"
#include "internal/swapchain.hpp"
#include "internal/resources.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
    #define NK_INCLUDE_FIXED_TYPES
    #define NK_INCLUDE_STANDARD_IO
    #define NK_INCLUDE_STANDARD_VARARGS
    #define NK_INCLUDE_DEFAULT_ALLOCATOR
    #define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
    #define NK_INCLUDE_FONT_BAKING
    #define NK_INCLUDE_DEFAULT_FONT
    #define NK_IMPLEMENTATION
    #define NK_GLFW_VULKAN_IMPLEMENTATION
    #include <nuklear/nuklear.h>
    #include <nuklear/nuklear_glfw_vulkan.h>
    #include <nuklear/overview.c>
#pragma clang diagnostic pop

using namespace renderer;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    log::unbuffered(std::format("{}", pCallbackData->pMessage), log::level::VULKAN);
    return VK_FALSE;
}

bool renderer::init(RendererState* state) {
    // create instance
    vkb::InstanceBuilder instanceBuilder;
    auto vkbInstance = instanceBuilder
        .set_app_name(config::APP_NAME.c_str())
        .request_validation_layers(config::renderer::ENABLE_VALIDATION_LAYERS)
        .set_debug_callback(debugCallback)
        .require_api_version(1, 3, 0)
        .build()
        .value();
    state->instance = vkbInstance.instance;
    state->debugMessenger = vkbInstance.debug_messenger;
    state->deinitStack.emplace_back([state] {
        auto destroyDbgMsger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(state->instance, "vkDestroyDebugUtilsMessengerEXT");
        destroyDbgMsger(state->instance, state->debugMessenger, nullptr);
		vkDestroyInstance(state->instance, nullptr);
    });

    // create surface
    vkCheck(glfwCreateWindowSurface(state->instance, state->engine->window, nullptr, &state->surface));

    // query physical device and create device
    auto physDevSelector = vkb::PhysicalDeviceSelector{ vkbInstance };
    auto vkbPhysDev = physDevSelector
        .set_minimum_version(1, 3)
        .set_required_features_13({
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .synchronization2 = true,
            .dynamicRendering = true,
        })
        .set_required_features_12({
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .descriptorIndexing = true,
            .shaderUniformBufferArrayNonUniformIndexing = true,
            .shaderSampledImageArrayNonUniformIndexing = true,
            .shaderStorageBufferArrayNonUniformIndexing = true,
            .descriptorBindingUniformBufferUpdateAfterBind = true,
            .descriptorBindingSampledImageUpdateAfterBind = true,
            .descriptorBindingStorageImageUpdateAfterBind = true,
            .descriptorBindingStorageBufferUpdateAfterBind = true,
            .descriptorBindingPartiallyBound = true,
            .runtimeDescriptorArray = true,
            .bufferDeviceAddress = true,
        })
        .add_required_extensions({
            "VK_EXT_mesh_shader",
            "VK_EXT_shader_object",
            "VK_KHR_acceleration_structure",
            "VK_KHR_deferred_host_operations",
        })
        .add_required_extension_features(VkPhysicalDeviceAccelerationStructureFeaturesKHR{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
            .descriptorBindingAccelerationStructureUpdateAfterBind = true,
        })
        .set_surface(state->surface)
        .select()
        .value();
    auto vkbDevice = vkb::DeviceBuilder{ vkbPhysDev }.build().value();
    state->device = vkbDevice.device;
    state->physicalDevice = vkbPhysDev.physical_device;

    // NOTE: surface must be destroyed b4 device
    state->deinitStack.emplace_back([state] {
        vkDestroySurfaceKHR(state->instance, state->surface, nullptr);
        vkDestroyDevice(state->device, nullptr);
    });

    // init vma
    VmaAllocatorCreateInfo vmaInfo = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = state->physicalDevice,
        .device = state->device,
        .instance = state->instance,
    };
    vmaCreateAllocator(&vmaInfo, &state->allocator);
    state->deinitStack.emplace_back([state] {
        vmaDestroyAllocator(state->allocator);
    });

    // get queues
    state->queue = {
        .graphics = vkbDevice.get_queue(vkb::QueueType::graphics).value(),
        .compute = vkbDevice.get_queue(vkb::QueueType::compute).value(),
        .transfer = vkbDevice.get_queue(vkb::QueueType::transfer).value(),
    };
    state->queueFamily = {
        .graphics = vkbDevice.get_queue_index(vkb::QueueType::graphics).value(),
        .compute = vkbDevice.get_queue_index(vkb::QueueType::compute).value(),
        .transfer = vkbDevice.get_queue_index(vkb::QueueType::transfer).value(),
    };
    
    // init swapchain
    auto [w, h] = utility::getWindowSize(state->engine);
    swapchain::create(state, w, h);
    state->deinitStack.emplace_back([state] {
        swapchain::destroy(state);
    });

    // init cmds
    auto cmdPoolInfo = vkinit::cmdPoolCreateInfo(state->queueFamily.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (usize i = 0; i < config::renderer::FRAME_OVERLAP; i++) {
		vkCheck(vkCreateCommandPool(state->device, &cmdPoolInfo, nullptr, &state->frames[i].cmdPool));
		auto cmdAllocInfo = vkinit::cmdBufferAllocInfo(state->frames[i].cmdPool, 1);
		vkCheck(vkAllocateCommandBuffers(state->device, &cmdAllocInfo, &state->frames[i].primaryCmdBuffer));
	}
    // immediate cmd
    vkCheck(vkCreateCommandPool(state->device, &cmdPoolInfo, nullptr, &state->immediate.cmdPool));
	auto cmdAllocInfo = vkinit::cmdBufferAllocInfo(state->immediate.cmdPool, 1);
	vkCheck(vkAllocateCommandBuffers(state->device, &cmdAllocInfo, &state->immediate.cmdBuffer));
	state->deinitStack.emplace_back([state] { 
	    vkDestroyCommandPool(state->device, state->immediate.cmdPool, nullptr);
	});

    // init sync structures
    auto fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    auto semCreateInfo = vkinit::semaphoreCreateInfo();
    for (usize i = 0;  i < config::renderer::FRAME_OVERLAP; i++) {
        vkCheck(vkCreateFence(state->device, &fenceCreateInfo, nullptr, &state->frames[i].renderFence));
        vkCheck(vkCreateSemaphore(state->device, &semCreateInfo, nullptr, &state->frames[i].swapchainSemaphore));
        vkCheck(vkCreateSemaphore(state->device, &semCreateInfo, nullptr, &state->frames[i].renderSemaphore));
    }
    // immediate fence
    vkCheck(vkCreateFence(state->device, &fenceCreateInfo, nullptr, &state->immediate.fence));
	state->deinitStack.emplace_back([state] {
        vkDestroyFence(state->device, state->immediate.fence, nullptr);
    });

    // deinit all per frame data
    state->deinitStack.emplace_back([state] {
        for (usize i = 0;  i < config::renderer::FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(state->device, state->frames[i].cmdPool, nullptr);
            vkDestroyFence(state->device, state->frames[i].renderFence, nullptr);
            vkDestroySemaphore(state->device, state->frames[i].renderSemaphore, nullptr);
            vkDestroySemaphore(state->device, state->frames[i].swapchainSemaphore, nullptr);
            utility::flushDeinitStack(&state->frames[i].deinitStack);
        }
    });

    descriptors::init(state);
    vkutil::initPipelineLayout(state);

    vkutil::loadFuckassVkFns(state);

    // create draw image
    //auto [ maxW, maxH ] = utility::getMonitorRes(state->engine);
    state->drawImage.image = res::allocateImage(state->allocator, w, h, res::STORAGE_IMAGE_USES);
    state->drawImage.view = res::createImageView(state, state->drawImage.image);
    state->drawImage.id = descriptors::registerStorageImage(state, state->drawImage.view);
	state->deinitStack.emplace_back([state] {
		vkDestroyImageView(state->device, state->drawImage.view, nullptr);
        res::deallocateImage(state->allocator, state->drawImage.image);
	});

    // create depth stencil
    //auto [ maxW, maxH ] = utility::getMonitorRes(state->engine);
    state->depthStencil.image = res::allocateImage(state->allocator, w, h, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    state->depthStencil.view = res::createImageView(state, state->depthStencil.image);
    state->depthStencil.id = descriptors::registerStorageImage(state, state->depthStencil.view);
	state->deinitStack.emplace_back([state] {
		vkDestroyImageView(state->device, state->depthStencil.view, nullptr);
        res::deallocateImage(state->allocator, state->depthStencil.image);
	});

    //ui::init(state);

    state->initialised = true;
    return true;
}

void renderer::deinit(RendererState* state) {
    vkDeviceWaitIdle(state->device);
    utility::flushDeinitStack(&state->deinitStack);
    state->initialised = false;
}

void buildCommandBuffer(RendererState* state, VkCommandBuffer cmd, u32 swapchainImageIndex) {

	//vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->globalPipelineLayout, 0, 1, &state->globalDescriptorSet, 0, nullptr);
	//vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, state->globalPipelineLayout, 0, 1, &state->globalDescriptorSet, 0, nullptr);

    auto cmdBeginInfo = vkinit::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    state->drawExtent.width = state->drawImage.image.extent.width;
    state->drawExtent.height = state->drawImage.image.extent.height;

    vkCheck(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    {
        // transition color and depth images for drawing
        vkutil::insertImageMemoryBarrier(
            cmd, state->swapchainImages[swapchainImageIndex], 0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        );
        vkutil::insertImageMemoryBarrier(
            cmd, state->depthStencil.image.image, 0,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 }
        );

        VkClearValue clearColor;
        clearColor.color = { {0.0f, 0.0f, 0.0f, 0.0f} };
        auto colorInfo = vkinit::attachmentInfo(state->swapchainImageViews[swapchainImageIndex], &clearColor);
        
        auto depthStencilInfo = vkinit::attachmentInfo(state->swapchainImageViews[swapchainImageIndex], nullptr, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        depthStencilInfo.clearValue.depthStencil = { 1.0f,  0 };

        auto [w, h] = utility::getWindowSize(state->engine);
        auto renderingInfo = vkinit::renderingInfo({w, h}, &colorInfo, &depthStencilInfo, &depthStencilInfo);

        state->vkCmdBeginRenderingKHR(cmd, &renderingInfo);

        VkViewport viewport = { .width = (f32)w, .height = (f32)h, .minDepth = 0.f, .maxDepth = 1.f };
        VkRect2D scissor = { .offset = { 0, 0 }, .extent = {w, h} };

        state->vkCmdSetViewportWithCountEXT(cmd, 1, &viewport);
        state->vkCmdSetScissorWithCountEXT(cmd, 1, &scissor);
        state->vkCmdSetCullModeEXT(cmd, VK_CULL_MODE_BACK_BIT);
        state->vkCmdSetFrontFaceEXT(cmd, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        state->vkCmdSetDepthTestEnableEXT(cmd, VK_TRUE);
        state->vkCmdSetDepthWriteEnableEXT(cmd, VK_TRUE);
        state->vkCmdSetDepthCompareOpEXT(cmd, VK_COMPARE_OP_LESS_OR_EQUAL);
        state->vkCmdSetPrimitiveTopologyEXT(cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        state->vkCmdSetRasterizerDiscardEnableEXT(cmd, VK_FALSE);
        state->vkCmdSetPolygonModeEXT(cmd, VK_POLYGON_MODE_FILL);
        state->vkCmdSetRasterizationSamplesEXT(cmd, VK_SAMPLE_COUNT_1_BIT);
        state->vkCmdSetAlphaToCoverageEnableEXT(cmd, VK_FALSE);
        state->vkCmdSetDepthBiasEnableEXT(cmd, VK_FALSE);
        state->vkCmdSetStencilTestEnableEXT(cmd, VK_FALSE);
        state->vkCmdSetPrimitiveRestartEnableEXT(cmd, VK_FALSE);

        const uint32_t sampleMask = 0xFF;
        state->vkCmdSetSampleMaskEXT(cmd, VK_SAMPLE_COUNT_1_BIT, &sampleMask);

        const VkBool32 colorBlendEnables = false;
        const VkColorComponentFlags colorBlendComponentFlags = 0xf;
        //const VkColorBlendEquationEXT colorBlendEquation{};
        state->vkCmdSetColorBlendEnableEXT(cmd, 0, 1, &colorBlendEnables);
        state->vkCmdSetColorWriteMaskEXT(cmd, 0, 1, &colorBlendComponentFlags);

        VkVertexInputBindingDescription2EXT vertexInputBinding{};
        vertexInputBinding.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
        vertexInputBinding.binding = 0;
        vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertexInputBinding.stride = sizeof(Vertex);
        vertexInputBinding.divisor = 1;

        std::vector<VkVertexInputAttributeDescription2EXT> vertexAttributes = {
            { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
            { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) },
            { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) }
        };

        state->vkCmdSetVertexInputEXT(cmd, 1, &vertexInputBinding, 3, vertexAttributes.data());





        state->vkCmdEndRenderingKHR(cmd);
    }
	vkCheck(vkEndCommandBuffer(cmd));
}

void renderer::draw(RendererState* state) {
    descriptors::updatePending(state);
    //ui::startFrame(state);

    // wait on gpu to finish rendering last frame
    vkCheck(vkWaitForFences(state->device, 1, &getCurrentFrame(state).renderFence, true, 1000000000 /*max 1 second timeout*/));
    vkCheck(vkResetFences(state->device, 1, &getCurrentFrame(state).renderFence));

    utility::flushDeinitStack(&getCurrentFrame(state).deinitStack);

    // request image from swapchain
    u32 swapchainImageIndex;
    vkCheck(vkAcquireNextImageKHR(
        state->device, state->swapchain,
        1000000000 /*max 1 second timeout*/,
        getCurrentFrame(state).swapchainSemaphore,
        nullptr, &swapchainImageIndex
    ));

    auto cmd = getCurrentFrame(state).primaryCmdBuffer;
    vkCheck(vkResetCommandBuffer(cmd, 0));

    buildCommandBuffer(state, cmd, swapchainImageIndex);
    
    // submit cmd buffer to queue to execute
    auto cmdInfo = vkinit::cmdBufferSubmitInfo(cmd);	
	auto waitInfo = vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame(state).swapchainSemaphore);
	auto signalInfo = vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame(state).renderSemaphore);
	auto submitInfo = vkinit::submitInfo(&cmdInfo, &signalInfo, &waitInfo);
	vkCheck(vkQueueSubmit2(state->queue.graphics, 1, &submitInfo, getCurrentFrame(state).renderFence));

    // prepare and present
	VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &getCurrentFrame(state).renderSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &state->swapchain,
        .pImageIndices = &swapchainImageIndex,
    };
	vkCheck(vkQueuePresentKHR(state->queue.graphics, &presentInfo));

	state->frameNumber++;
}
