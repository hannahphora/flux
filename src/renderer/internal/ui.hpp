#pragma once
#include "../renderer.hpp"
#include "vkstructs.hpp"
#include "helpers.hpp"

// silence clang for external includes
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
    #include <imgui/imgui.h>
    #include <imgui/backends/imgui_impl_glfw.h>
    #include <imgui/backends/imgui_impl_vulkan.h>
#pragma clang diagnostic pop

namespace flux::renderer::ui {

    void init(RendererState* state) {
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
        };
        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 1000,
            .poolSizeCount = (uint32_t)std::size(poolSizes),
            .pPoolSizes = poolSizes,
        };
        VK_CHECK(vkCreateDescriptorPool(state->device, &poolInfo, nullptr, &state->imguiDescriptorPool));

        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(state->engine->window, true);

        ImGui_ImplVulkan_InitInfo initInfo = {
            .Instance = state->instance,
            .PhysicalDevice = state->physicalDevice,
            .Device = state->device,
            .Queue = state->queue.graphics,
            .DescriptorPool = state->imguiDescriptorPool,
            .MinImageCount = 3,
            .ImageCount = 3,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,

            // dynamic rendering parameters
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &state->swapchainImageFormat,
            },
        };
        ImGui_ImplVulkan_Init(&initInfo);

        state->deinitStack.emplace_back([state] {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            vkDestroyDescriptorPool(state->device, state->imguiDescriptorPool, nullptr);
        });
    }

    void startFrame(RendererState* state) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		ImGui::Render();
    }

    void draw(RendererState* state, VkCommandBuffer cmd, VkImageView targetImageView) {
        auto colorAttachment = vkstruct::attachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        auto renderInfo = vkstruct::renderingInfo(state->swapchainExtent, &colorAttachment, nullptr, nullptr);

        vkCmdBeginRendering(cmd, &renderInfo);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        vkCmdEndRendering(cmd);
    }
}