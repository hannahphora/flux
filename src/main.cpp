#include <common.h>
#include <core/engine.h>

i32 main(i32 argc, char** argv) {
    // TODO: parse cmd line args

    auto engine = new EngineState;
    engine::init(engine);
    engine::run(engine);
    engine::deinit(engine);
    delete engine;

    return 0;
}