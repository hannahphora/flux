#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>

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

using usize = unsigned long long;

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

namespace flux {
    enum class Systems {
        ENGINE,
        RENDERER,
        INPUT,
        AUDIO,
        PHYSICS,
        NETWORKING,
        AI,
    };

    namespace engine { struct EngineState; }
    namespace renderer { struct RendererState; }
    namespace input { struct InputState; }
    namespace audio { struct AudioState; }
    namespace physics { struct PhysicsState; }
    namespace networking { struct NetworkingState; }
    namespace ai { struct AiState; }
}

using namespace flux;
using flux::engine::EngineState;
using flux::renderer::RendererState;
using flux::input::InputState;
using flux::audio::AudioState;
using flux::physics::PhysicsState;
using flux::networking::NetworkingState;
using flux::ai::AiState;

using DeinitStack = std::vector<std::function<void()>>;
