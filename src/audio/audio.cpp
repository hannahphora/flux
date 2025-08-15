#include "audio.hpp"

using namespace audio;

bool audio::init(AudioState* state) {
    state->initialised = true;
    return true;
}

void audio::deinit(AudioState* state) {
    state->initialised = false;
}

void audio::update(AudioState* state) {
    
}