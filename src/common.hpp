#pragma once

//---------------------------------------------------
// |>~ C STD LIB ~<|
//---------------------------------------------------

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>

//---------------------------------------------------
// |>~ C++ STD LIB ~<|
//---------------------------------------------------

#include <atomic>
#include <array>
#include <chrono>
#include <string>
#include <vector>
#include <functional>
#include <limits>
#include <thread>
#include <random>
#include <fstream>
#include <filesystem>
#include <span>
#include <unordered_map>

//---------------------------------------------------
// |>~ BASE TYPES ~<|
//---------------------------------------------------

using usize = size_t;

using i8 = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using f32 = float;
using f64 = double;

//---------------------------------------------------
// |>~ OTHER TYPES ~<|
//---------------------------------------------------

using DeinitStack = std::vector<std::function<void()>>;

//---------------------------------------------------
// |>~ FLUX NAMESPACE / SYSTEMS ~<|
//---------------------------------------------------

namespace flux {
    enum class Systems {
        ENGINE,
        RENDERER,
        INPUT,
    };

    namespace engine { struct EngineState; }
    namespace renderer { struct RendererState; }
    namespace input { struct InputState; }
}

using namespace flux;
using flux::engine::EngineState;
using flux::renderer::RendererState;
using flux::input::InputState;
