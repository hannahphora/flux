#pragma once
#include "../renderer.hpp"
#include "vkstructs.hpp"
#include "helpers.hpp"
#include "images.hpp"

namespace flux::renderer::pipelines {

    void initLayout(RendererState* state, const u32 pushConstantSize = 128) {
        VkPushConstantRange pushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset = 0U,
            .size = pushConstantSize,
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

    struct PipelineBuilder {
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        VkPipelineInputAssemblyStateCreateInfo inputAssembly;
        VkPipelineRasterizationStateCreateInfo rasterizer;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo multisampling;
        VkPipelineLayout pipelineLayout;
        VkPipelineDepthStencilStateCreateInfo depthStencil;
        VkPipelineRenderingCreateInfo renderInfo;
        VkFormat colorAttachmentformat;

        PipelineBuilder() { clear(); }

        void clear() {
            inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
            rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
            colorBlendAttachment = {};
            multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
            pipelineLayout = {};
            depthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
            renderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
            shaderStages.clear();
        }

        VkPipeline build(VkDevice device) {
            // make viewport state from stored viewport and scissor, currently multiple viewports or scissors not supported
            VkPipelineViewportStateCreateInfo viewportState = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .scissorCount = 1
            };

            // dummy color blending, as no transparent objects yet
            VkPipelineColorBlendStateCreateInfo colorBlending = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = 1,
                .pAttachments = &colorBlendAttachment,
            };

            VkPipelineVertexInputStateCreateInfo vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

            VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            VkPipelineDynamicStateCreateInfo dynamicInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = 2,
                .pDynamicStates = &state[0],
            };

            VkGraphicsPipelineCreateInfo pipelineInfo = {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &renderInfo,
                .stageCount = (u32)shaderStages.size(),
                .pStages = shaderStages.data(),
                .pVertexInputState = &vertexInputInfo,
                .pInputAssemblyState = &inputAssembly,
                .pViewportState = &viewportState,
                .pRasterizationState = &rasterizer,
                .pMultisampleState = &multisampling,
                .pDepthStencilState = &depthStencil,
                .pColorBlendState = &colorBlending,
                .pDynamicState = &dynamicInfo,
                .layout = pipelineLayout,
            };

            VkPipeline pipeline = {};
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
                log::warn("failed to create graphics pipeline");
                return VK_NULL_HANDLE;
            } else {
                return pipeline;
            }
        }

        void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader) {
            shaderStages.clear();
            shaderStages.emplace_back(vkstruct::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
            shaderStages.emplace_back(vkstruct::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
        }

        void setInputTopology(VkPrimitiveTopology topology) {
            inputAssembly.topology = topology;
            inputAssembly.primitiveRestartEnable = VK_FALSE; // primitive restart not in use
        }

        void setPolygonMode(VkPolygonMode mode) {
            rasterizer.polygonMode = mode;
            rasterizer.lineWidth = 1.f;
        }

        void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
            rasterizer.cullMode = cullMode;
            rasterizer.frontFace = frontFace;
        }

        void setMultisamplingNone() {
            multisampling.sampleShadingEnable = VK_FALSE;
            // multisampling defaulted to none (1 sample ppx)
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampling.minSampleShading = 1.0f;
            multisampling.pSampleMask = nullptr;
            multisampling.alphaToCoverageEnable = VK_FALSE; // no alpha to coverage
            multisampling.alphaToOneEnable = VK_FALSE;
        }

        void disableBlending() {
            colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE; // no blending
        }

        void setColorAttachmentFormat(VkFormat format) {
            colorAttachmentformat = format;
            // connect format to the renderInfo struct
            renderInfo.colorAttachmentCount = 1;
            renderInfo.pColorAttachmentFormats = &colorAttachmentformat;
        }

        void setDepthFormat(VkFormat format) {
            renderInfo.depthAttachmentFormat = format;
        }

        void disableDepthtest() {
            depthStencil.depthTestEnable = VK_FALSE;
            depthStencil.depthWriteEnable = VK_FALSE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;
            depthStencil.front = {};
            depthStencil.back = {};
            depthStencil.minDepthBounds = 0.f;
            depthStencil.maxDepthBounds = 1.f;
        }
    };

}