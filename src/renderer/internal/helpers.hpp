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
}
