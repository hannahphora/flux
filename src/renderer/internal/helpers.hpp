#pragma once
#include "../renderer.hpp"
#include "vkstructs.hpp"

namespace flux::renderer {

    #define VK_CHECK(result) {\
        if (result != VK_SUCCESS) {\
            log::unbuffered(std::format("Vulkan error: {} at {}:{}", string_VkResult(result), __FILE__, __LINE__), log::level::ERROR);\
            utility::exitWithFailure();\
        }\
    }

    inline FrameData& getCurrentFrame(RendererState* state) {
        return state->frames[state->frameNumber % config::renderer::FRAME_OVERLAP];
    };

}

namespace flux::renderer::vkutil {

    void immediateSubmit(RendererState* state, std::function<void(VkCommandBuffer cmd)>&& fn) {
        VK_CHECK(vkResetFences(state->device, 1, &state->immediateSubmit.fence));
        VK_CHECK(vkResetCommandBuffer(state->immediateSubmit.cmdBuffer, 0));

        VkCommandBuffer cmd = state->immediateSubmit.cmdBuffer;
        auto cmdBeginInfo = vkstruct::cmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
        fn(cmd);
        VK_CHECK(vkEndCommandBuffer(cmd));

        // NOTE: render fence will now block until graphics cmds finish execution
        auto cmdInfo = vkstruct::cmdBufferSubmitInfo(cmd);
        auto submit = vkstruct::submitInfo(&cmdInfo, nullptr, nullptr);
        VK_CHECK(vkQueueSubmit2(state->queue.graphics, 1, &submit, state->immediateSubmit.fence));
        VK_CHECK(vkWaitForFences(state->device, 1, &state->immediateSubmit.fence, true, 9999999999));
    }
}
