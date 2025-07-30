#include <common/math.hpp>

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

template <typename T> T math::rndInt(const T min, const T max) {
    std::mt19937 gen(randomDevice());
    return std::uniform_int_distribution<T>{ min, max }(gen);
}