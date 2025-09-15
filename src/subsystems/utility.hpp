#pragma once
#include <common.hpp>

namespace flux::utility {
    void flushDeinitStack(DeinitStack* deinitStack);
    std::pair<u32, u32> getWindowSize(const EngineState* state);
    std::pair<u32, u32> getMonitorRes(const EngineState* state);
    void sleepMs(u32 ms);

    [[noreturn]] void abort();
    [[noreturn]] void exitWithFailure();
    [[noreturn]] void exitWithSuccess();
}