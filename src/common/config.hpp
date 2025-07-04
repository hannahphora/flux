#pragma once
#include <common/types.hpp>

#include <cstdlib>
#include <stdio.h>
#include <cstring>
#include <string>
#include <vector>
#include <functional>

namespace flux::config {
//--------------------------------------------------------------------------------------------

namespace general {
    static const std::string ENGINE_NAME = "flux";
    static const std::string APP_NAME = "TestApplication";
};

namespace log {
    static FILE* OUTPUT_FILE = stderr;
    static constexpr usize BUFFER_SIZE = 8 * 1024 * 1024; // 8mb
};

namespace renderer {
    static constexpr bool ENABLE_VALIDATION_LAYERS = true;
};

namespace input {
    
};

//--------------------------------------------------------------------------------------------
};