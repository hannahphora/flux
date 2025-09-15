#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"
#include "helpers.hpp"
#include "images.hpp"

namespace flux::renderer::resources {

    static constexpr VkImageUsageFlags STORAGE_IMAGE_USES =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    static constexpr VkImageUsageFlags COMBINED_SAMPLER_USES =
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    AllocatedBuffer allocateBuffer(VmaAllocator allocator, usize size, VkBufferUsageFlags usage, VmaMemoryUsage memUsage) {
        AllocatedBuffer result = {};
        VkBufferCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
        };
        VmaAllocationCreateInfo allocInfo = {
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = memUsage,
        };
        vkCheck(vmaCreateBuffer(allocator, &info, &allocInfo, &result.buffer, &result.allocation, &result.info));
        return result;
    }

    void deallocateBuffer(VmaAllocator allocator, const AllocatedBuffer& buffer) {
        vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
    }

    AllocatedImage allocateImage(VmaAllocator allocator, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false) {
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
        vkCheck(vmaCreateImage(allocator, &info, &allocInfo, &result.image, &result.allocation, nullptr));
        return result;
    }

    void deallocateImage(VmaAllocator allocator, const AllocatedImage& image) {
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
        vkCheck(vkCreateImageView(state->device, &info, nullptr, &result));
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
        vkCheck(vkCreateSampler(state->device, &info, nullptr, &result));
        return result;
    }
}