#include <core/engine.hpp>

int main() {
    EngineState* state = new EngineState;
    engine::init(state);

    state->running = true;
    while (state->running) {
        engine::process_frame(state);


    }

    engine::deinit(state);
    delete state;
    return EXIT_SUCCESS;
}