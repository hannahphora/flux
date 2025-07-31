#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"

#include <common/utility.hpp>

namespace flux::renderer::ui {

    void layoutDebugWindow(RendererState* state) {
        if (ImGui::Begin("debug")) {

            ImGui::Text("haii :3");
        }
        ImGui::End();
    }

    void loadImguiContext(RendererState* state) {
        ImGui_ImplGlfw_RestoreCallbacks(state->engine->window);
        ImGui::SetCurrentContext(state->imguiContext);

        ImGuiMemAllocFunc allocFn;
        ImGuiMemFreeFunc freeFn;
        void* userData;
        ImGui::GetAllocatorFunctions(&allocFn, &freeFn, &userData);
        ImGui::SetAllocatorFunctions(allocFn, freeFn, userData);

        state->imguiContext->IO.BackendRendererUserData = state->imguiVulkanData;
    }

    void init(RendererState* state) {
        // imgui descriptor pool (currently very oversize)
        VkDescriptorPoolSize imguiPoolSizes[] = {
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
        VkDescriptorPoolCreateInfo imguiPoolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 1000,
            .poolSizeCount = (u32)std::size(imguiPoolSizes),
            .pPoolSizes = imguiPoolSizes,
        };
        vkCheck(vkCreateDescriptorPool(state->device, &imguiPoolInfo, nullptr, &state->imguiDescriptorPool));

        state->imguiContext = ImGui::CreateContext();

        ImGui_ImplGlfw_InitForVulkan(state->engine->window, true);
        ImGui_ImplVulkan_InitInfo imguiInitInfo = {
            .Instance = state->instance,
            .PhysicalDevice = state->physicalDevice,
            .Device = state->device,
            .Queue = state->graphicsQueue,
            .DescriptorPool = state->imguiDescriptorPool,
            .MinImageCount = 3,
            .ImageCount = 3,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo = { 
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &state->swapchainImageFormat,
            },
        };
        ImGui_ImplVulkan_Init(&imguiInitInfo);
        state->deinitStack.emplace_back([state] {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            vkDestroyDescriptorPool(state->device, state->imguiDescriptorPool, nullptr);
            delete state->imguiContext;
        });

        state->imguiVulkanData = state->imguiContext->IO.BackendRendererUserData;
        loadImguiContext(state);
    }

    void draw(RendererState* state, VkCommandBuffer cmd, VkImageView targetImageView) {
        auto colorAttachment = vkinit::attachmentInfo(targetImageView, nullptr);
        auto renderInfo = vkinit::renderingInfo(state->swapchainExtent, &colorAttachment, nullptr);

        vkCmdBeginRendering(cmd, &renderInfo);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        vkCmdEndRendering(cmd);
    }

    void startFrame(RendererState* state) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        layoutDebugWindow(state);
        ImGui::Render();
    }
}