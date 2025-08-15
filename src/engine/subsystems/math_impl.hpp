#pragma once
#include "math.hpp"

namespace flux::math {
    static std::random_device randomDevice;
}

bool math::cmpF32(const f32 a, const f32 b, const f32 threshold) {
    return fabsf(a - b) <= (FLT_EPSILON + threshold) * fmaxf(1.f, fmaxf(fabsf(a), fabsf(b)));
}

f32 math::rndF32(const f32 min, const f32 max) {
    std::mt19937 gen(randomDevice());
    return std::uniform_real_distribution{ min, max }(gen);
}

u32 math::rndU32(const u32 min, const u32 max) {
    std::mt19937 gen(randomDevice());
    return std::uniform_int_distribution<u32>{ min, max }(gen);
}