#pragma once
#include "../renderer.hpp"
#include "helpers.hpp"
#include "vkstructs.hpp"

namespace flux::renderer::vkres {

    static constexpr VkImageUsageFlags STORAGE_IMAGE_USES =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    static constexpr VkImageUsageFlags COMBINED_SAMPLER_USES =
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    AllocatedImage createImage(VmaAllocator allocator, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false) {
        AllocatedImage result = {
            .extent = size,
            .format = format,
        };
        VkImageCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = result.format,
            .extent = result.extent,
            .mipLevels = (mipmapped) ? static_cast<u32>(std::floor(std::log2(std::max(size.width, size.height)))) + 1 : 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT, // for MSAA, default to 1 sample ppx
            .tiling = VK_IMAGE_TILING_OPTIMAL, // OPTIMAL for smallest size on gpu, LINEAR for cpu readback
            .usage = usage,
        };
        VmaAllocationCreateInfo allocInfo = { // gpu local memory
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = (VkMemoryPropertyFlags)VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };
        VK_CHECK(vmaCreateImage(allocator, &info, &allocInfo, &result.image, &result.allocation, nullptr));
        return result;
    }

    void destroyImage(VmaAllocator allocator, const AllocatedImage& image) {
        vmaDestroyImage(allocator, image.image, image.allocation);
    }

    VkImageView createImageView(RendererState* state, const AllocatedImage& img) {
        VkImageView result = nullptr;
        VkImageViewCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = img.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = img.format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        VK_CHECK(vkCreateImageView(state->device, &info, nullptr, &result));
        return result;
    }

    VkSampler createSampler(RendererState* state, const AllocatedImage& img) {
        VkSampler result = nullptr;
        VkSamplerCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .anisotropyEnable = true,
            .maxAnisotropy = 1.0,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        };
        VK_CHECK(vkCreateSampler(state->device, &info, nullptr, &result));
        return result;
    }

}

namespace flux::renderer::vkutil {

    void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout) {
        
        VkImageMemoryBarrier2 imageBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .oldLayout = currentLayout,
            .newLayout = newLayout,
        };

        imageBarrier.subresourceRange = vkstruct::imageSubresourceRange(
            (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
        imageBarrier.image = image;

        VkDependencyInfo depInfo = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
        };
        vkCmdPipelineBarrier2(cmd, &depInfo);
    }

    void copyImageToImage(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize) {
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

    void insertImageMemoryBarrier(
			VkCommandBuffer cmd,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange)
    {
        auto imageMemoryBarrier = vkstruct::imageMemoryBarrier();
        imageMemoryBarrier.srcAccessMask = srcAccessMask;
        imageMemoryBarrier.dstAccessMask = dstAccessMask;
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        vkCmdPipelineBarrier(cmd, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }
}