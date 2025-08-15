#include "ai.hpp"

using namespace ai;

bool ai::init(AiState* state) {
    state->initialised = true;
    return true;
}

void ai::deinit(AiState* state) {
    state->initialised = false;
}

void ai::update(AiState* state) {
    
}