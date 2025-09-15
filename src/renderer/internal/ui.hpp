#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"
#include "helpers.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

namespace flux::renderer::ui {

    void init(RendererState* state) {
        // 1: create descriptor pool for IMGUI
        //  the size of the pool is very oversize, but it's copied from imgui demo
        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 1000,
            .poolSizeCount = (uint32_t)std::size(pool_sizes),
            .pPoolSizes = pool_sizes,
        };
        VkDescriptorPool imguiPool;
        vkCheck(vkCreateDescriptorPool(state->device, &poolInfo, nullptr, &imguiPool));

        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(state->engine->window, true);

        ImGui_ImplVulkan_InitInfo initInfo = {
            .Instance = state->instance,
            .PhysicalDevice = state->physicalDevice,
            .Device = state->device,
            .Queue = state->queue.graphics,
            .DescriptorPool = imguiPool,
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
        //ImGui_ImplVulkan_CreateFontsTexture();

        state->deinitStack.emplace_back([&] {
            ImGui_ImplVulkan_Shutdown();
            vkDestroyDescriptorPool(state->device, imguiPool, nullptr);
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
        auto colorAttachment = initializers::attachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        auto renderInfo = initializers::renderingInfo(state->swapchainExtent, &colorAttachment, nullptr, nullptr);

        vkCmdBeginRendering(cmd, &renderInfo);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        vkCmdEndRendering(cmd);
    }
}