#pragma once
#include <common.hpp>

namespace flux::physics {
    bool init(PhysicsState* state);
    void deinit(PhysicsState* state);
    void update(PhysicsState* state);

    struct PhysicsState {
        const EngineState* engine;
        bool initialised = false;
    };
}