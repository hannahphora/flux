#pragma once
#include <common.hpp>

namespace flux::ai {
    bool init(AiState* state);
    void deinit(AiState* state);
    void update(AiState* state);

    struct AiState {
        const EngineState* engine;
        bool initialised = false;
    };
}