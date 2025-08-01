#pragma once
#include <common/common.hpp>

namespace flux::utility {
    void flushDeinitStack(DeinitStack* deinitStack);
    std::pair<u32, u32> getWindowSize(const EngineState* state);
    std::pair<u32, u32> getMonitorRes(const EngineState* state);
    void sleepFor(u32 ms);
}