#pragma once
#include <common/common.hpp>

namespace flux::utility {
    void flushDeinitStack(DeinitStack* deinitStack);
    std::pair<u32, u32> getWindowSize(const EngineState* state);
}