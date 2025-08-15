#pragma once
#include <common/common.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace flux::math {
    bool cmpF32(f32 a, f32 b, f32 threshold  = 0.0001f);
    f32 rndF32(f32 min = 0.0f, f32 max = 1.0f);
    u32 rndU32(u32 min, u32 max);
}