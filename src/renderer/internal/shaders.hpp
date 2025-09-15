#pragma once
#include "../renderer.hpp"
#include "vkstructs.hpp"
#include "helpers.hpp"

namespace flux::renderer::vkutil {

    bool loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule) {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) return false;
        usize fileSize = (usize)file.tellg();
        std::vector<u32> buffer(fileSize / sizeof(u32)); // spirv expects u32 buffer
        file.seekg(0);
        file.read((char*)buffer.data(), (i64)fileSize);
        file.close();

        VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = buffer.size() * sizeof(u32),
            .pCode = buffer.data(),
        };

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            return false;
        }
        *outShaderModule = shaderModule;
        return true;
    }

}