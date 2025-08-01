#pragma once
#include "../renderer.hpp"

namespace flux::renderer::vkinit {
    
    inline VkCommandPoolCreateInfo cmdPoolCreateInfo(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
        return {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = flags,
            .queueFamilyIndex = queueFamilyIndex,
        };
    }

    inline VkCommandBufferAllocateInfo cmdBufferAllocInfo(VkCommandPool pool, u32 count = 1) {
        return {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = count,
        };
    }

    inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0) {
        return {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = flags,
        };
    }

    inline VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0) {
        return {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .flags = flags,
        };
    }

    inline VkCommandBufferBeginInfo cmdBufferBeginInfo(VkCommandBufferUsageFlags flags = 0) {
        return {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = flags,
            .pInheritanceInfo = nullptr,
        };
    }

    inline VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
        return {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = semaphore,
            .value = 1,
            .stageMask = stageMask,
            .deviceIndex = 0,
        };
    }

    inline VkCommandBufferSubmitInfo cmdBufferSubmitInfo(VkCommandBuffer cmd) {
        return {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = cmd,
            .deviceMask = 0,
        };
    }

    inline VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalInfo, VkSemaphoreSubmitInfo* waitInfo) {
        return {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = (waitInfo == nullptr) ? 0U : 1U,
            .pWaitSemaphoreInfos = waitInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = cmd,
            .signalSemaphoreInfoCount = (signalInfo == nullptr) ? 0U : 1U,
            .pSignalSemaphoreInfos = signalInfo,
        };
    }

    inline VkRenderingAttachmentInfo attachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        return {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = view,
            .imageLayout = layout,
            .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = (clear) ? *clear : VkClearValue{},
        };
    }

    inline VkRenderingInfo renderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* color, VkRenderingAttachmentInfo* depth, VkRenderingAttachmentInfo* stencil) {
        return {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { { 0, 0 }, renderExtent },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = color,
            .pDepthAttachment = depth,
            .pStencilAttachment = stencil,
        };
    }

    inline VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask) {
        return {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        };
    }

    inline VkImageMemoryBarrier imageMemoryBarrier() {
        return {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        };
    }
}
