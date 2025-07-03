#pragma once
#include <common.h>

namespace flux::config {
//--------------------------------------------------------------------------------------------

namespace general {
    static const std::string ENGINE_NAME = "flux";
    static const std::string APP_NAME = "TestApplication";
};

namespace log {
    static FILE* OUTPUT_FILE = stdout;
    static constexpr usize BUFFER_SIZE = 8 * 1024;
};

namespace renderer {
    static constexpr bool ENABLE_VALIDATION_LAYERS = true;
};

namespace input {
    
};

//--------------------------------------------------------------------------------------------
};