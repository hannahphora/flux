#pragma once
#include "../renderer.hpp"

namespace flux::renderer::vkinit {
    
    VkCommandPoolCreateInfo cmdPoolCreateInfo(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
        return {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = flags,
            .queueFamilyIndex = queueFamilyIndex,
        };
    }

    VkCommandBufferAllocateInfo cmdBufferAllocInfo(VkCommandPool pool, u32 count = 1) {
        return {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = count,
        };
    }

    VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0) {
        return {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = flags,
        };
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0) {
        return {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .flags = flags,
        };
    }

    VkCommandBufferBeginInfo cmdBufferBeginInfo(VkCommandBufferUsageFlags flags = 0) {
        return {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = flags,
            .pInheritanceInfo = nullptr,
        };
    }

    VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
        return {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = semaphore,
            .value = 1,
            .stageMask = stageMask,
            .deviceIndex = 0,
        };
    }

    VkCommandBufferSubmitInfo cmdBufferSubmitInfo(VkCommandBuffer cmd) {
        return {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = cmd,
            .deviceMask = 0,
        };
    }

    VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalInfo, VkSemaphoreSubmitInfo* waitInfo) {
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
}
