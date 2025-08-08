#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"

namespace flux::renderer::descriptors {

    void init(RendererState* state) {
        static constexpr std::array<VkDescriptorType, 5> types = {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        };
        std::array<VkDescriptorSetLayoutBinding, types.size()> bindings = {};
        std::array<VkDescriptorBindingFlags, types.size()> bindingFlags = {};
        std::array<VkDescriptorPoolSize, types.size()> poolSizes = {};

        // descriptor set layout
        for (u32 i = 0; i < bindings.size(); i++) {
            bindings[i] = {
                .binding = i,
                .descriptorType = types[i],
                .descriptorCount = config::renderer::MAX_DESCRIPTOR_COUNT,
                .stageFlags = VK_SHADER_STAGE_ALL,
            };
            bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
            poolSizes[i] = { types[i], config::renderer::MAX_DESCRIPTOR_COUNT };
        }
        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = bindingFlags.size(),
            .pBindingFlags = bindingFlags.data(),
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &bindingFlagsInfo,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = bindings.size(),
            .pBindings = bindings.data(),
        };
        vkCreateDescriptorSetLayout(state->device, &descriptorSetLayoutCreateInfo, nullptr, &state->globalDescriptorSetLayout);

        // descriptor pool
        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 1,
            .poolSizeCount = poolSizes.size(),
            .pPoolSizes = poolSizes.data(),
        };
        vkCreateDescriptorPool(state->device, &poolInfo, nullptr, &state->globalDescriptorPool);

        // descriptor set
        VkDescriptorSetAllocateInfo allocateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = state->globalDescriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &state->globalDescriptorSetLayout,
        };
        vkAllocateDescriptorSets(state->device, &allocateInfo, &state->globalDescriptorSet);

        state->deinitStack.emplace_back([state] {
            vkDestroyDescriptorPool(state->device, state->globalDescriptorPool, nullptr);
            vkDestroyDescriptorSetLayout(state->device, state->globalDescriptorSetLayout, nullptr);
        });
    }

    void updatePending(RendererState* state) {
        vkUpdateDescriptorSets(state->device, (u32)state->pendingWriteDescriptors.write.size(), state->pendingWriteDescriptors.write.data(), 0, nullptr);
        state->pendingWriteDescriptors.write.clear();
        state->pendingWriteDescriptors.info.clear();
    }

    StorageBufferId registerStorageBuffer(RendererState* state, VkBuffer buffer, VkDeviceSize size) {
        StorageBufferId result;
        if (!state->availableDescriptorId.storageBuffer.empty()) {
            result = state->availableDescriptorId.storageBuffer.back();
            state->availableDescriptorId.storageBuffer.pop_back();
        }
        else result = (StorageBufferId)state->nextAvailableDecriptorId.storageBuffer++;
        state->pendingWriteDescriptors.write.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = state->globalDescriptorSet,
            .dstBinding = (u32)Binding::STORAGE_BUFFER,
            .dstArrayElement = (u32)result,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        });
        state->pendingWriteDescriptors.info.emplace_back();
        state->pendingWriteDescriptors.info.back().buffer = {
            .buffer = buffer,
            .offset = 0,
            .range = size,
        };
        state->pendingWriteDescriptors.write.back().pBufferInfo = &state->pendingWriteDescriptors.info.back().buffer;
        return result;
    }

    CombinedSamplerId registerCombinedSampler(RendererState* state, VkImageView view, VkSampler sampler) {
        CombinedSamplerId result;
        if (!state->availableDescriptorId.combinedSampler.empty()) {
            result = state->availableDescriptorId.combinedSampler.back();
            state->availableDescriptorId.combinedSampler.pop_back();
        }
        else result = (CombinedSamplerId)state->nextAvailableDecriptorId.combinedSampler++;
        state->pendingWriteDescriptors.write.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = state->globalDescriptorSet,
            .dstBinding = (u32)Binding::COMBINED_SAMPLER,
            .dstArrayElement = (u32)result,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        });
        state->pendingWriteDescriptors.info.emplace_back();
        state->pendingWriteDescriptors.info.back().image = {
            .sampler = sampler,
            .imageView = view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        state->pendingWriteDescriptors.write.back().pImageInfo = &state->pendingWriteDescriptors.info.back().image;
        return result;
    }

    StorageImageId registerStorageImage(RendererState* state, VkImageView view) {
        StorageImageId result;
        if (!state->availableDescriptorId.storageImage.empty()) {
            result = state->availableDescriptorId.storageImage.back();
            state->availableDescriptorId.storageImage.pop_back();
        }
        else result = (StorageImageId)state->nextAvailableDecriptorId.storageImage++;
        state->pendingWriteDescriptors.write.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = state->globalDescriptorSet,
            .dstBinding = (u32)Binding::STORAGE_IMAGE,
            .dstArrayElement = (u32)result,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        });
        state->pendingWriteDescriptors.info.emplace_back();
        state->pendingWriteDescriptors.info.back().image = {
            .imageView = view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        state->pendingWriteDescriptors.write.back().pImageInfo = &state->pendingWriteDescriptors.info.back().image;
        return result;
    }

}