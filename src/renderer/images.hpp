#pragma once
#include "renderer.hpp"
#include "helpers.hpp"

namespace flux::renderer::vkinit {

    VkImageSubresourceRange imgSubresourceRange(VkImageAspectFlags aspectMask) {
        return {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        };
    }

    VkImageCreateInfo imgCreateInfo(VkFormat fmt, VkImageUsageFlags usageFlags, VkExtent3D extent) {
        return {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = fmt,
            .extent = extent,
            .mipLevels = 1,
            .arrayLayers = 1,
            // for MSAA, default to 1 sample ppx
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL, // OPTIMAL for smallest size on gpu, LINEAR for cpu readback
            .usage = usageFlags,
        };
    }

    VkImageViewCreateInfo imgViewCreateInfo(VkFormat fmt, VkImage img, VkImageAspectFlags aspectFlags) {
        // build a img view for the depth img to use for rendering
        return {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = img,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = fmt,
            .subresourceRange = {
                .aspectMask = aspectFlags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
    }
}

namespace flux::renderer::vkutil {
    void transitionImg(VkCommandBuffer cmd, VkImage img,
        VkImageLayout currentLayout, VkImageLayout newLayout) {
        
        VkImageMemoryBarrier2 imgBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .oldLayout = currentLayout,
            .newLayout = newLayout,
        };

        imgBarrier.subresourceRange = vkinit::imgSubresourceRange(
            (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
        imgBarrier.image = img;

        VkDependencyInfo depInfo = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imgBarrier,
        };
        vkCmdPipelineBarrier2(cmd, &depInfo);
    }

    void copyImgToImg(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize) {
        VkImageBlit2 blitRegion = { 
            .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        blitRegion.srcOffsets[1] = {
            .x = (i32)srcSize.width,
            .y = (i32)srcSize.height,
            .z = 1,
        };
        blitRegion.dstOffsets[1] = {
            .x = (i32)dstSize.width,
            .y = (i32)dstSize.height,
            .z = 1,
        };
        VkBlitImageInfo2 blitInfo = {
            .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
            .srcImage = src,
            .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage = dst,
            .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .regionCount = 1,
            .pRegions = &blitRegion,
            .filter = VK_FILTER_LINEAR,
        };
        vkCmdBlitImage2(cmd, &blitInfo);
    }
}