#pragma once
#include "../renderer.hpp"

using namespace renderer;

bool ui::init(UiState* state) {
    state->initialised = true;
    return true;
}

void ui::deinit(UiState* state) {
    utility::flushDeinitStack(&state->deinitStack);
    state->initialised = false;
}
