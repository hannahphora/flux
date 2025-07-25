#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>

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

namespace flux { struct EngineState; };
namespace flux { struct RendererState; };
namespace flux { struct InputState; };
namespace flux { struct UiState; };
namespace flux { struct AudioState; };
namespace flux { struct PhysicsState; };
namespace flux { struct AnimationState; };
namespace flux { struct NetworkingState; };

using namespace flux;

using DeinitStack = std::vector<std::function<void()>>;
