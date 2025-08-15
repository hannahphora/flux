#pragma once
#include <common/common.hpp>

// silence clang for external includes
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
    
#pragma clang diagnostic pop

namespace flux::physics {
    bool init(PhysicsState* state);
    void deinit(PhysicsState* state);
    void update(PhysicsState* state);

    struct PhysicsState {
        const EngineState* engine;
        bool initialised = false;
    };
}