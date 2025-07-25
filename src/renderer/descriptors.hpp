#pragma once
#include "renderer.hpp"
#include "helpers.hpp"

namespace flux::renderer::vkutil {

    struct DescriptorAllocator {
        struct PoolSizeRatio {
            VkDescriptorType type = {};
            f32 ratio = {};
        } pool;

        void initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {

        }

        void clearDescriptors(VkDevice device) {

        }

        void destroyPool(VkDevice device) {
            
        }

        VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
    };

    struct DescriptorLayoutBuilder {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {};

        void addBinding(u32 binding, VkDescriptorType type) {
            bindings.push_back({
                .binding = binding,
                .descriptorCount = 1,
                .descriptorType = type,
            });
        }

        void clear() {
            bindings.clear();
        }

        VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages,
            void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0)
        {
            for (auto& b : bindings)
                b.stageFlags |= shaderStages;

            VkDescriptorSetLayoutCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = pNext,
                .pBindings = bindings.data(),
                .bindingCount = (uint32_t)bindings.size(),
                .flags = flags,
            };

            VkDescriptorSetLayout set = {};
            vkCheck(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));
            return set;
        }
    };

}