#define VMA_IMPLEMENTATION
#include "renderer.hpp"

#include <core/engine.hpp>

#include <common/log.hpp>
#include <common/utility.hpp>

#include "internal/images.hpp"
#include "internal/helpers.hpp"
#include "internal/ui_impl.hpp"

using namespace renderer;

bool renderer::init(RendererState* state) {
    // create instance
    vkb::InstanceBuilder instBuilder;
    auto vkbInst = instBuilder
        .set_app_name(config::APP_NAME.c_str())
        .request_validation_layers(config::renderer::ENABLE_VALIDATION_LAYERS)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build()
        .value();
    state->instance = vkbInst.instance;
    state->dbgMessenger = vkbInst.debug_messenger;
    state->deinitStack.emplace_back([state] {
        vkb::destroy_debug_utils_messenger(state->instance, state->dbgMessenger);
		vkDestroyInstance(state->instance, nullptr);
    });

    // create surface
    vkCheck(glfwCreateWindowSurface(state->instance, state->engine->window, nullptr, &state->surface));

    // query physical device and create device
    auto physDevSelector = vkb::PhysicalDeviceSelector{ vkbInst };
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
        .set_surface(state->surface)
        .select()
        .value();
    auto devBuilder = vkb::DeviceBuilder{ vkbPhysDev };
    auto vkbDevice = devBuilder.build().value();
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
    state->graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    state->computeQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
    state->transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
    state->graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    state->computeQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::compute).value();
    state->transferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();
    
    // init swapchain & draw img
    auto [w, h] = utility::getWindowSize(state->engine);
    createSwapchain(state, w, h);
    state->deinitStack.emplace_back([state] {
        destroySwapchain(state);
    });
    createDrawImage(state, w, h);
	state->deinitStack.emplace_back([state] {
		vkDestroyImageView(state->device, state->drawImage.view, nullptr);
		vmaDestroyImage(state->allocator, state->drawImage.image, state->drawImage.allocation);
	});

    // init cmds
    auto cmdPoolInfo = vkinit::cmdPoolCreateInfo(
        state->graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
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

    // init descriptors set
    std::array<VkDescriptorSetLayoutBinding, 3> descriptorBindings = {};
    std::array<VkDescriptorBindingFlags, 3> descriptorBindingFlags = {};
    std::array<VkDescriptorPoolSize, 3> descriptorPoolSizes = {};
    std::array<VkDescriptorType, 3> descriptorTypes = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    };
    // descriptor set layout
    for (u32 i = 0; i < descriptorBindings.size(); i++) {
        descriptorBindings[i] = {
            .binding = i,
            .descriptorType = descriptorTypes[i],
            .descriptorCount = config::renderer::MAX_DESCRIPTOR_COUNT,
            .stageFlags = VK_SHADER_STAGE_ALL,
        };
        descriptorBindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        descriptorPoolSizes[i] = { descriptorTypes[i], config::renderer::MAX_DESCRIPTOR_COUNT };
    }
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = descriptorBindingFlags.size(),
        .pBindingFlags = descriptorBindingFlags.data(),
    };
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &bindingFlagsInfo,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .bindingCount = descriptorBindings.size(),
        .pBindings = descriptorBindings.data(),
    };
    vkCreateDescriptorSetLayout(state->device, &descriptorSetLayoutCreateInfo, nullptr, &state->bindlessDescriptorSetLayout);
    // descriptor pool
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = 1,
        .poolSizeCount = descriptorPoolSizes.size(),
        .pPoolSizes = descriptorPoolSizes.data(),
    };
    vkCreateDescriptorPool(state->device, &poolInfo, nullptr, &state->descriptorPool);
    // descriptor set
    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = state->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &state->bindlessDescriptorSetLayout,
    };
    vkAllocateDescriptorSets(state->device, &allocateInfo, &state->bindlessDescriptorSet);
    state->deinitStack.emplace_back([state] {
        vkDestroyDescriptorPool(state->device, state->descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(state->device, state->bindlessDescriptorSetLayout, nullptr);
    });

    // create draw img descriptor
    state->drawImageID = (TextureID)0;
    initDrawImageDescriptor(state);

    // init pipelines
    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset = 0U,
        .size = 128U,
    };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &state->bindlessDescriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };
    vkCreatePipelineLayout(state->device, &pipelineLayoutInfo, nullptr, &state->pipelineLayout);
    state->deinitStack.emplace_back([state] {
        vkDestroyPipelineLayout(state->device, state->pipelineLayout, nullptr);
		vkDestroyPipeline(state->device, state->gradientPipeline, nullptr);
    });

    // create gradient pipeline
    VkShaderModule computeDrawShader;
	if (!loadShaderModule("zig-out/bin/res/shaders/gradient.spv", state->device, &computeDrawShader))
        log::unbuffered("error creating compute shader module", log::level::WARNING);
	VkPipelineShaderStageCreateInfo stageinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = computeDrawShader,
        .pName = "main",
    };
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = stageinfo,
        .layout = state->pipelineLayout,
    };
	vkCheck(vkCreateComputePipelines(state->device, nullptr, 1, &computePipelineCreateInfo, nullptr, &state->gradientPipeline));
    vkDestroyShaderModule(state->device, computeDrawShader, nullptr);
	state->deinitStack.emplace_back([state] {
		vkDestroyPipeline(state->device, state->gradientPipeline, nullptr);
    });

    // init ui
    log::buffered("initialising ui");
    if (!ui::init(state->ui = new UiState { .engine = state->engine }))
        log::unbuffered("failed to init ui", log::level::ERROR);
    state->deinitStack.emplace_back([state] {
        log::buffered("deinitialising ui");
        ui::deinit(state->ui);
        delete state->ui;
    });

    state->initialised = true;
    return true;
}

void renderer::deinit(RendererState* state) {
    vkDeviceWaitIdle(state->device);
    utility::flushDeinitStack(&state->deinitStack);
    state->initialised = false;
}

void drawBg(RendererState* state, VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, state->gradientPipeline);
	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        state->pipelineLayout,
        0, 1, &state->bindlessDescriptorSet,
        0, nullptr
    );
	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(state->drawExtent.width / 16.0), std::ceil(state->drawExtent.height / 16.0), 1);
}

void renderer::draw(RendererState* state) {
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

    // reset cmd buffer
    auto cmd = getCurrentFrame(state).primaryCmdBuffer;
    vkCheck(vkResetCommandBuffer(cmd, 0));
    auto cmdBeginInfo = vkinit::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    state->drawExtent.width = state->drawImage.extent.width;
    state->drawExtent.height = state->drawImage.extent.height;

    // records cmds
    vkCheck(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    {
        // transition main draw img to general layout for writing
        vkutil::transitionImage(cmd, state->drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        drawBg(state, cmd);

        // transition draw img and swapchain img to transfer layouts
        vkutil::transitionImage(cmd, state->drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkutil::transitionImage(cmd, state->swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // execute a copy from draw img into the swapchain
        vkutil::copyImageToImage(cmd, state->drawImage.image, state->swapchainImages[swapchainImageIndex], state->drawExtent, state->swapchainExtent);

        // set swapchain img layout to present
        vkutil::transitionImage(cmd, state->swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
	vkCheck(vkEndCommandBuffer(cmd));

    // submit cmd buffer to queue to execute
    auto cmdInfo = vkinit::cmdBufferSubmitInfo(cmd);	
	auto waitInfo = vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame(state).swapchainSemaphore);
	auto signalInfo = vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame(state).renderSemaphore);
	auto submitInfo = vkinit::submitInfo(&cmdInfo, &signalInfo, &waitInfo);
	vkCheck(vkQueueSubmit2(state->graphicsQueue, 1, &submitInfo, getCurrentFrame(state).renderFence));

    // prepare and present
	VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &getCurrentFrame(state).renderSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &state->swapchain,
        .pImageIndices = &swapchainImageIndex,
    };
	vkCheck(vkQueuePresentKHR(state->graphicsQueue, &presentInfo));

	state->frameNumber++;
}
