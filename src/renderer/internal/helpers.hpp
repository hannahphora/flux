#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"

namespace flux::renderer {

    inline void vkCheck(VkResult result) {
        if (result) {
            log::unbuffered(std::format("Vulkan error: {}", string_VkResult(result)), log::level::ERROR);
            utility::exitWithFailure();
        }
    }

    inline FrameData& getCurrentFrame(RendererState* state) {
        return state->frames[state->frameNumber % config::renderer::FRAME_OVERLAP];
    };

}

namespace flux::renderer::vkutil {

    void immediateSubmit(RendererState* state, std::function<void(VkCommandBuffer cmd)>&& fn) {
        vkCheck(vkResetFences(state->device, 1, &state->immediate.fence));
        vkCheck(vkResetCommandBuffer(state->immediate.cmdBuffer, 0));

        VkCommandBuffer cmd = state->immediate.cmdBuffer;
        auto cmdBeginInfo = initializers::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        vkCheck(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
        fn(cmd);
        vkCheck(vkEndCommandBuffer(cmd));

        // NOTE: render fence will now block until graphics cmds finish execution
        auto cmdInfo = initializers::cmdBufferSubmitInfo(cmd);
        auto submit = initializers::submitInfo(&cmdInfo, nullptr, nullptr);
        vkCheck(vkQueueSubmit2(state->queue.graphics, 1, &submit, state->immediate.fence));
        vkCheck(vkWaitForFences(state->device, 1, &state->immediate.fence, true, 9999999999));
    }
}
