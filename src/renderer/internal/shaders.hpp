#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"
#include "helpers.hpp"

namespace flux::renderer::shader {
    Shader create(RendererState* state, const char* name, const char* path, VkShaderStageFlagBits stage, VkShaderStageFlags nextStage) {
        Shader result = {
            .name = name,
            .stage = stage,
            .nextStage = nextStage,
        };

        // load spirv
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            log::unbuffered("try and compile shader from src at runtime if spirv not found", log::level::TODO);
            log::unbuffered(std::format("couldnt find shader at {}", path), log::level::ERROR);
        }
        usize size = (usize)file.tellg();
        std::vector<u32> spirv(size / sizeof(u32));
        file.seekg(0);
        file.read((char*)spirv.data(), size);
        file.close();
        result.spirv = std::move(spirv);

        static constexpr VkPushConstantRange pushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset = 0U,
            .size = config::renderer::PUSH_CONSTANT_SIZE,
        };
        result.createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
            .flags = NULL,
            .stage = stage,
            .nextStage = nextStage,
            .codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT,
            .codeSize = spirv.size() * sizeof(u32), // size in bytes
            .pCode = result.spirv.data(),
            .pName = "main",
            .setLayoutCount = 1,
            .pSetLayouts = &state->globalDescriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
        };
        return result;
    }
    
    void buildLinked(RendererState* state, Shader* vert, Shader* frag) {
        if (vert == nullptr || frag == nullptr)
            log::unbuffered("building linked shaders failed with null input", log::level::WARNING);
        
        VkShaderCreateInfoEXT shaderCreateInfos[2];
        shaderCreateInfos[0] = vert->createInfo;
        shaderCreateInfos[1] = frag->createInfo;
        for (auto& i : shaderCreateInfos)
            i.flags |= VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;

        VkShaderEXT shaders[2];
        vkCheck(vkCreateShadersEXT(state->device, 2, shaderCreateInfos, nullptr, shaders));
        vert->shader = shaders[0];
        frag->shader = shaders[1];
    }

    void build(RendererState* state, Shader* shader) {
        if (shader == nullptr)
            log::unbuffered("building shader failed with null input", log::level::WARNING);
        vkCheck(vkCreateShadersEXT(state->device, 1, &shader->createInfo, nullptr, &shader->shader));
    }

    void bind(VkCommandBuffer cmd, Shader* shader) {
        vkCmdBindShadersEXT(cmd, 1, &shader->stage, &shader->shader);
    }
}