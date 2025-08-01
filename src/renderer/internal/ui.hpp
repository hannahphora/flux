#pragma once
#include "../renderer.hpp"
#include "initializers.hpp"

#include <common/utility.hpp>

namespace flux::renderer::ui {

    //void init(RendererState* state) {
    //    state->nk.ctx = nk_glfw3_init(
    //        state->engine->window,
    //        state->device,
    //        state->physicalDevice,
    //        state->queueFamily.graphics,
    //        demo.overlay_image_views,
    //        demo.swap_chain_images_len,
    //        demo.swap_chain_image_format,
    //        NK_GLFW3_INSTALL_CALLBACKS,
    //        MAX_VERTEX_BUFFER,
    //        MAX_ELEMENT_BUFFER,
    //    );
    //    state->deinitStack.emplace_back([state] {
    //        nk_glfw3_shutdown();
    //        delete state->nk.ctx;
    //    });
    //    
    //    state->nk.img = nk_image_ptr(demo.demo_texture_image_view);
    //    state->nk.bg = {
    //        .r = 0.10f,
    //        .g = 0.18f,
    //        .b = 0.24f,
    //        .a = 1.0f,
    //    };
    //}

    //void startFrame(RendererState* state) {
    //    nk_glfw3_new_frame();
    //    if (nk_begin(state->nk.ctx, "Demo", nk_rect(50, 50, 230, 250),
    //            NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE
    //    )) {
    //        enum { EASY, HARD };
    //        static int op = EASY;
    //        static int property = 20;
    //        nk_layout_row_static(state->nk.ctx, 30, 80, 1);
    //        if (nk_button_label(state->nk.ctx, "button"))
    //            fprintf(stdout, "button pressed\n");
    //
    //        nk_layout_row_dynamic(state->nk.ctx, 30, 2);
    //        if (nk_option_label(state->nk.ctx, "easy", op == EASY))
    //            op = EASY;
    //        if (nk_option_label(state->nk.ctx, "hard", op == HARD))
    //            op = HARD;
    //
    //        nk_layout_row_dynamic(state->nk.ctx, 25, 1);
    //        nk_property_int(state->nk.ctx, "Compression:", 0, &property, 100, 10, 1);
    //
    //        nk_layout_row_dynamic(state->nk.ctx, 20, 1);
    //        nk_label(state->nk.ctx, "background:", NK_TEXT_LEFT);
    //        nk_layout_row_dynamic(state->nk.ctx, 25, 1);
    //        if (nk_combo_begin_color(state->nk.ctx, nk_rgb_cf(state->nk.bg),
    //                                 nk_vec2(nk_widget_width(state->nk.ctx), 400))) {
    //            nk_layout_row_dynamic(state->nk.ctx, 120, 1);
    //            state->nk.bg = nk_color_picker(state->nk.ctx, state->nk.bg, NK_RGBA);
    //            nk_layout_row_dynamic(state->nk.ctx, 25, 1);
    //            state->nk.bg.r = nk_propertyf(state->nk.ctx, "#R:", 0, state->nk.bg.r, 1.0f, 0.01f, 0.005f);
    //            state->nk.bg.g = nk_propertyf(state->nk.ctx, "#G:", 0, state->nk.bg.g, 1.0f, 0.01f, 0.005f);
    //            state->nk.bg.b = nk_propertyf(state->nk.ctx, "#B:", 0, state->nk.bg.b, 1.0f, 0.01f, 0.005f);
    //            state->nk.bg.a = nk_propertyf(state->nk.ctx, "#A:", 0, state->nk.bg.a, 1.0f, 0.01f, 0.005f);
    //            nk_combo_end(state->nk.ctx);
    //        }
    //    }
    //    nk_end(state->nk.ctx);
    //}
    //
    //void draw(RendererState* state, VkCommandBuffer cmd, VkImageView targetImageView) {
    //    if (nk_begin(state->nk.ctx, "Texture", nk_rect(500, 300, 200, 200),
    //            NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE
    //    )) {
    //        struct nk_command_buffer *canvas = nk_window_get_canvas(state->nk.ctx);
    //        struct nk_rect total_space = nk_window_get_content_region(state->nk.ctx);
    //        nk_draw_image(canvas, total_space, &state->nk.img, nk_white);
    //    }
    //    nk_end(state->nk.ctx);
    //}
}