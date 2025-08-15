#include "physics.hpp"

using namespace physics;

bool physics::init(PhysicsState* state) {
    state->initialised = true;
    return true;
}

void physics::deinit(PhysicsState* state) {
    state->initialised = false;
}

void physics::update(PhysicsState* state) {
    
}