#pragma once
#include "../renderer.hpp"
#include "helpers.hpp"
#include "vkstructs.hpp"

namespace flux::renderer::vkres {

    AllocatedBuffer createBuffer(VmaAllocator allocator, usize size, VkBufferUsageFlags usage, VmaMemoryUsage memUsage) {
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
        VK_CHECK(vmaCreateBuffer(allocator, &info, &allocInfo, &result.buffer, &result.allocation, &result.info));
        return result;
    }

    void destroyBuffer(VmaAllocator allocator, const AllocatedBuffer& buffer) {
        vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
    }

    // NOTE:
    // - this is currently very inefficient as its being done on the main thread,
    //   it would be a good idea to move to worker thread
    // - also currently the staging buffer is being recreated each time,
    //   that should instead be kept and reused
    GPUMeshBuffers uploadMesh(RendererState* state, std::span<u32> indices, std::span<Vertex> vertices) {
        const usize vertexBufferSize = vertices.size() * sizeof(Vertex);
        const usize indexBufferSize = indices.size() * sizeof(u32);

        GPUMeshBuffers meshBuffers = {
            .indexBuffer = createBuffer(state->allocator, indexBufferSize,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            ),
            .vertexBuffer = createBuffer(state->allocator, vertexBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            ),
        };

        // get vertex buffer address
        VkBufferDeviceAddressInfo deviceAdressInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = meshBuffers.vertexBuffer.buffer
        };
        meshBuffers.vertexBufferAddress = vkGetBufferDeviceAddress(state->device, &deviceAdressInfo);

        // create staging buffer
        AllocatedBuffer staging = createBuffer(state->allocator, vertexBufferSize + indexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        void* data = staging.allocation->GetMappedData();

        memcpy(data, vertices.data(), vertexBufferSize); // copy vertex buffer
        memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize); // copy index buffer

        vkutil::immediateSubmit(state, [&](VkCommandBuffer cmd) {
            VkBufferCopy vertexCopy = {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = vertexBufferSize,
            };
            vkCmdCopyBuffer(cmd, staging.buffer, meshBuffers.vertexBuffer.buffer, 1, &vertexCopy);

            VkBufferCopy indexCopy = {
                .srcOffset = vertexBufferSize,
                .dstOffset = 0,
                .size = indexBufferSize,
            };
            vkCmdCopyBuffer(cmd, staging.buffer, meshBuffers.indexBuffer.buffer, 1, &indexCopy);
        });

        destroyBuffer(state->allocator, staging);
        return meshBuffers;
    }

}