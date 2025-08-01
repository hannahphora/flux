#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"

namespace flux::renderer {

    void vkCheck(VkResult result) {
        if (result) log::unbuffered(std::format("Vulkan error: {}", string_VkResult(result)), log::level::ERROR);
    }

    FrameData& getCurrentFrame(RendererState* state) {
        return state->frames[state->frameNumber % config::renderer::FRAME_OVERLAP];
    };

}

namespace flux::renderer::vkutil {

    void immediateSubmit(RendererState* state, std::function<void(VkCommandBuffer cmd)>&& fn) {
        vkCheck(vkResetFences(state->device, 1, &state->immediate.fence));
        vkCheck(vkResetCommandBuffer(state->immediate.cmdBuffer, 0));

        VkCommandBuffer cmd = state->immediate.cmdBuffer;
        auto cmdBeginInfo = vkinit::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        vkCheck(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
        fn(cmd);
        vkCheck(vkEndCommandBuffer(cmd));

        // NOTE: render fence will now block until graphics cmds finish execution
        auto cmdInfo = vkinit::cmdBufferSubmitInfo(cmd);
        auto submit = vkinit::submitInfo(&cmdInfo, nullptr, nullptr);
        vkCheck(vkQueueSubmit2(state->queue.graphics, 1, &submit, state->immediate.fence));
        vkCheck(vkWaitForFences(state->device, 1, &state->immediate.fence, true, 9999999999));
    }

    void initPipelineLayout(RendererState* state) {
        VkPushConstantRange pushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset = 0U,
            .size = 128U,
        };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &state->globalDescriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
        };
        vkCreatePipelineLayout(state->device, &pipelineLayoutInfo, nullptr, &state->globalPipelineLayout);
        state->deinitStack.emplace_back([state] {
            vkDestroyPipelineLayout(state->device, state->globalPipelineLayout, nullptr);
        });
    }

    void loadFuckassVkFns(RendererState* state) {
        state->vkCreateShadersEXT = reinterpret_cast<PFN_vkCreateShadersEXT>(vkGetDeviceProcAddr(state->device, "vkCreateShadersEXT"));
		state->vkDestroyShaderEXT = reinterpret_cast<PFN_vkDestroyShaderEXT>(vkGetDeviceProcAddr(state->device, "vkDestroyShaderEXT"));
		state->vkCmdBindShadersEXT = reinterpret_cast<PFN_vkCmdBindShadersEXT>(vkGetDeviceProcAddr(state->device, "vkCmdBindShadersEXT"));
		state->vkGetShaderBinaryDataEXT = reinterpret_cast<PFN_vkGetShaderBinaryDataEXT>(vkGetDeviceProcAddr(state->device, "vkGetShaderBinaryDataEXT"));
		state->vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(state->device, "vkCmdBeginRenderingKHR"));
		state->vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(state->device, "vkCmdEndRenderingKHR"));
		state->vkCmdSetAlphaToCoverageEnableEXT = reinterpret_cast<PFN_vkCmdSetAlphaToCoverageEnableEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetAlphaToCoverageEnableEXT"));
		state->vkCmdSetColorBlendEnableEXT = reinterpret_cast<PFN_vkCmdSetColorBlendEnableEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetColorBlendEnableEXT"));
		state->vkCmdSetColorWriteMaskEXT = reinterpret_cast<PFN_vkCmdSetColorWriteMaskEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetColorWriteMaskEXT"));
		state->vkCmdSetCullModeEXT = reinterpret_cast<PFN_vkCmdSetCullModeEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetCullModeEXT"));
		state->vkCmdSetDepthBiasEnableEXT = reinterpret_cast<PFN_vkCmdSetDepthBiasEnableEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetDepthBiasEnableEXT"));
		state->vkCmdSetDepthCompareOpEXT = reinterpret_cast<PFN_vkCmdSetDepthCompareOpEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetDepthCompareOpEXT"));
		state->vkCmdSetDepthTestEnableEXT = reinterpret_cast<PFN_vkCmdSetDepthTestEnableEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetDepthTestEnableEXT"));
		state->vkCmdSetDepthWriteEnableEXT = reinterpret_cast<PFN_vkCmdSetDepthWriteEnableEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetDepthWriteEnableEXT"));
		state->vkCmdSetFrontFaceEXT = reinterpret_cast<PFN_vkCmdSetFrontFaceEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetFrontFaceEXT"));
		state->vkCmdSetPolygonModeEXT = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetPolygonModeEXT"));
		state->vkCmdSetPrimitiveRestartEnableEXT = reinterpret_cast<PFN_vkCmdSetPrimitiveRestartEnableEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetPrimitiveRestartEnableEXT"));
		state->vkCmdSetPrimitiveTopologyEXT = reinterpret_cast<PFN_vkCmdSetPrimitiveTopologyEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetPrimitiveTopologyEXT"));
		state->vkCmdSetRasterizationSamplesEXT = reinterpret_cast<PFN_vkCmdSetRasterizationSamplesEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetRasterizationSamplesEXT"));
		state->vkCmdSetRasterizerDiscardEnableEXT = reinterpret_cast<PFN_vkCmdSetRasterizerDiscardEnableEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetRasterizerDiscardEnableEXT"));
		state->vkCmdSetSampleMaskEXT = reinterpret_cast<PFN_vkCmdSetSampleMaskEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetSampleMaskEXT"));
		state->vkCmdSetScissorWithCountEXT = reinterpret_cast<PFN_vkCmdSetScissorWithCountEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetScissorWithCountEXT"));
		state->vkCmdSetStencilTestEnableEXT = reinterpret_cast<PFN_vkCmdSetStencilTestEnableEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetStencilTestEnableEXT"));
		state->vkCmdSetVertexInputEXT = reinterpret_cast<PFN_vkCmdSetVertexInputEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetVertexInputEXT"));
		state->vkCmdSetViewportWithCountEXT = reinterpret_cast<PFN_vkCmdSetViewportWithCountEXT>(vkGetDeviceProcAddr(state->device, "vkCmdSetViewportWithCountEXT"));;
    }
}
