#pragma once

#include <cstdlib>

namespace flux { struct EngineState; };
#ifndef USE_FLUX_NAMESPACE
using namespace flux;
#endif // USE_FLUX_NAMESPACE

// TYPES
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
