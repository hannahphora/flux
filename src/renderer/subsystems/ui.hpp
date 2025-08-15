#pragma once
#include <common.hpp>

#include <renderer/renderer.hpp>

namespace flux::renderer::ui {
    void init(RendererState* state);
    void startFrame(RendererState* state);
    void draw(RendererState* state, VkCommandBuffer cmd, VkImageView targetImageView);
}