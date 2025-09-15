#define VMA_IMPLEMENTATION
#include "renderer.hpp"

#include <core/engine.hpp>

#include "internal/helpers.hpp"
#include "internal/vkstructs.hpp"
#include "internal/images.hpp"
#include "internal/buffers.hpp"
#include "internal/descriptors.hpp"
#include "internal/swapchain.hpp"
#include "internal/pipelines.hpp"
#include "internal/shaders.hpp"
#include "internal/ui.hpp"

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
    VK_CHECK(glfwCreateWindowSurface(state->instance, state->engine->window, nullptr, &state->surface));

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
    auto cmdPoolInfo = vkstruct::cmdPoolCreateInfo(state->queueFamily.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (usize i = 0; i < config::renderer::FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateCommandPool(state->device, &cmdPoolInfo, nullptr, &state->frames[i].cmdPool));
		auto cmdAllocInfo = vkstruct::cmdBufferAllocInfo(state->frames[i].cmdPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(state->device, &cmdAllocInfo, &state->frames[i].primaryCmdBuffer));
	}
    // immediate cmd
    VK_CHECK(vkCreateCommandPool(state->device, &cmdPoolInfo, nullptr, &state->immediateSubmit.cmdPool));
	auto cmdAllocInfo = vkstruct::cmdBufferAllocInfo(state->immediateSubmit.cmdPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(state->device, &cmdAllocInfo, &state->immediateSubmit.cmdBuffer));
	state->deinitStack.emplace_back([state] { 
	    vkDestroyCommandPool(state->device, state->immediateSubmit.cmdPool, nullptr);
	});

    // init sync structures
    auto fenceCreateInfo = vkstruct::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    auto semCreateInfo = vkstruct::semaphoreCreateInfo();
    for (usize i = 0;  i < config::renderer::FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(state->device, &fenceCreateInfo, nullptr, &state->frames[i].renderFence));
        VK_CHECK(vkCreateSemaphore(state->device, &semCreateInfo, nullptr, &state->frames[i].swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(state->device, &semCreateInfo, nullptr, &state->frames[i].renderSemaphore));
    }
    // immediate fence
    VK_CHECK(vkCreateFence(state->device, &fenceCreateInfo, nullptr, &state->immediateSubmit.fence));
	state->deinitStack.emplace_back([state] {
        vkDestroyFence(state->device, state->immediateSubmit.fence, nullptr);
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
    
    // create draw image
    //auto [ maxW, maxH ] = utility::getMonitorRes(state->engine);
    state->drawImage.image = vkres::createImage(
        state->allocator,
        { w, h, 1 },
        VK_FORMAT_R16G16B16A16_SFLOAT,
        vkres::STORAGE_IMAGE_USES
    );
    state->drawImage.view = vkres::createImageView(state, state->drawImage.image);
    state->drawImage.id = descriptors::registerStorageImage(state, state->drawImage.view);
	state->deinitStack.emplace_back([state] {
		vkDestroyImageView(state->device, state->drawImage.view, nullptr);
        vkres::destroyImage(state->allocator, state->drawImage.image);
	});

    // create pipeline
    VkShaderModule fragShader = {};
	if (!vkutil::loadShaderModule("zig-out/bin/res/shaders/coloredTriangle.frag.spv", state->device, &fragShader))
        log::warn("error building triangle fragment shader module");
	else
        log::debug("triangle fragment shader succesfully loaded");
    
	VkShaderModule vertShader = {};
	if (!vkutil::loadShaderModule("zig-out/bin/res/shaders/coloredTriangle.vert.spv", state->device, &vertShader))
        log::warn("error building triangle vertex shader module");
	else
        log::debug("triangle vertex shader succesfully loaded");
	
    pipelines::initLayout(state, sizeof(GPUDrawPushConstants));

    pipelines::PipelineBuilder pipelineBuilder;
	pipelineBuilder.pipelineLayout = state->globalPipelineLayout;               // use global pipeline layout
	pipelineBuilder.setShaders(vertShader, fragShader);                         // connect vert and frag shaders
	pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);      // draw triangles
	pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);                       // filled triangles
	pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);    // no backface culling
	pipelineBuilder.setMultisamplingNone();                                     // no multisampling
	pipelineBuilder.disableBlending();                                          // no blending
	pipelineBuilder.disableDepthtest();                                         // no depth testing
	pipelineBuilder.setColorAttachmentFormat(state->drawImage.image.format);    // connect draw img format
	pipelineBuilder.setDepthFormat(VK_FORMAT_UNDEFINED);                        // currently no depth img
	state->pipeline = pipelineBuilder.build(state->device);             // build pipeline

	// cleanup
	vkDestroyShaderModule(state->device, fragShader, nullptr);
	vkDestroyShaderModule(state->device, vertShader, nullptr);
	state->deinitStack.emplace_back([state] { vkDestroyPipeline(state->device, state->pipeline, nullptr); });

    ui::init(state);

    state->initialised = true;
    return true;
}

void renderer::deinit(RendererState* state) {
    vkDeviceWaitIdle(state->device);
    utility::flushDeinitStack(&state->deinitStack);
    state->initialised = false;
}

void drawGeometry(RendererState* state, VkCommandBuffer cmd) {
    // begin a render pass with draw image
	auto colorAttachment = vkstruct::attachmentInfo(state->drawImage.view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto renderInfo = vkstruct::renderingInfo(state->drawExtent, &colorAttachment, nullptr, nullptr);
	vkCmdBeginRendering(cmd, &renderInfo);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipeline);

	// set dynamic viewport
	VkViewport viewport = {
        .x = 0.f,
        .y = 0.f,
        .width = (f32)state->drawExtent.width,
        .height = (f32)state->drawExtent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
	vkCmdSetViewport(cmd, 0, 1, &viewport);

    // set scissor
	VkRect2D scissor = {
        .offset = { .x = 0, .y = 0 },
        .extent = {
            .width = state->drawExtent.width,
            .height = state->drawExtent.height
        },
    };
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// launch a draw command to draw 3 vertices
	vkCmdDraw(cmd, 3, 1, 0, 0);
	vkCmdEndRendering(cmd);
}

void buildCommandBuffer(RendererState* state, VkCommandBuffer cmd, u32 swapchainImageIndex) {

	//vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->globalPipelineLayout, 0, 1, &state->globalDescriptorSet, 0, nullptr);
	//vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, state->globalPipelineLayout, 0, 1, &state->globalDescriptorSet, 0, nullptr);

    auto cmdBeginInfo = vkstruct::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    state->drawExtent.width = state->drawImage.image.extent.width;
    state->drawExtent.height = state->drawImage.image.extent.height;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    {
        vkutil::transitionImage(cmd, state->drawImage.image.image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        vkutil::transitionImage(cmd, state->drawImage.image.image,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        drawGeometry(state, cmd);

        // transtion the draw image and the swapchain image into their correct transfer layouts
        vkutil::transitionImage(cmd, state->drawImage.image.image,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkutil::transitionImage(cmd, state->swapchainImages[swapchainImageIndex],
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // execute a copy from the draw image into the swapchain
	    vkutil::copyImageToImage(cmd, state->drawImage.image.image, state->swapchainImages[swapchainImageIndex], state->drawExtent, state->swapchainExtent);

        // set swapchain image layout to Attachment Optimal so we can draw it
        vkutil::transitionImage(cmd, state->swapchainImages[swapchainImageIndex],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // draw imgui into the swapchain image
        ui::draw(state, cmd, state->swapchainImageViews[swapchainImageIndex]);

        // set swapchain image layout to Present so we can draw it
        vkutil::transitionImage(cmd, state->swapchainImages[swapchainImageIndex],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
	VK_CHECK(vkEndCommandBuffer(cmd));
}

void renderer::draw(RendererState* state) {
    descriptors::updatePending(state);
    ui::startFrame(state);

    // wait on gpu to finish rendering last frame
    VK_CHECK(vkWaitForFences(state->device, 1, &getCurrentFrame(state).renderFence, true, 1000000000 /*max 1 second timeout*/));
    VK_CHECK(vkResetFences(state->device, 1, &getCurrentFrame(state).renderFence));

    utility::flushDeinitStack(&getCurrentFrame(state).deinitStack);

    // request image from swapchain
    u32 swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(
        state->device, state->swapchain,
        1000000000 /*max 1 second timeout*/,
        getCurrentFrame(state).swapchainSemaphore,
        nullptr, &swapchainImageIndex
    ));

    auto cmd = getCurrentFrame(state).primaryCmdBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    buildCommandBuffer(state, cmd, swapchainImageIndex);
    
    // submit cmd buffer to queue to execute
    auto cmdInfo = vkstruct::cmdBufferSubmitInfo(cmd);	
	auto waitInfo = vkstruct::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame(state).swapchainSemaphore);
	auto signalInfo = vkstruct::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame(state).renderSemaphore);
	auto submitInfo = vkstruct::submitInfo(&cmdInfo, &signalInfo, &waitInfo);
	VK_CHECK(vkQueueSubmit2(state->queue.graphics, 1, &submitInfo, getCurrentFrame(state).renderFence));

    // prepare and present
	VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &getCurrentFrame(state).renderSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &state->swapchain,
        .pImageIndices = &swapchainImageIndex,
    };
	VK_CHECK(vkQueuePresentKHR(state->queue.graphics, &presentInfo));

	state->frameNumber++;
}
