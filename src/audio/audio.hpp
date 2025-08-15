#pragma once
#include <common.hpp>

namespace flux::audio {
    bool init(AudioState* state);
    void deinit(AudioState* state);
    void update(AudioState* state);

    struct AudioState {
        const EngineState* engine;
        bool initialised = false;
    };
}