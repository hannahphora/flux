#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"

// BINDLESS DESCRIPTOR TYPES
//  - uniform
//  - storage buffer
//  - texture (combined sampler)
//  - image (storage image)
//  - acceleration structure

namespace flux::renderer::vkutil {

    void initDescriptors(RendererState* state) {

        std::array<VkDescriptorType, 5> types = {
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

}