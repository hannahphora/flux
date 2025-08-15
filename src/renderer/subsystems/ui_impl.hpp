#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"

#include <common/utility.hpp>

namespace flux::renderer::ui {

    void init(RendererState* state) {}

    void startFrame(RendererState* state) {}
    
    void draw(RendererState* state, VkCommandBuffer cmd, VkImageView targetImageView) {}
}