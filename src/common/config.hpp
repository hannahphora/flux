#pragma once
#include <common/common.hpp>

namespace flux::config {

    static const std::string ENGINE_NAME = "flux";
    static const std::string APP_NAME = "TestApplication";

    static constexpr usize WINDOW_WIDTH = 800;
    static constexpr usize WINDOW_HEIGHT = 600;

    namespace log {
        static FILE* OUTPUT_FILE = stderr;
        static constexpr usize BUFFER_SIZE = 8 * 1024 * 1024; // 8mb
        static constexpr usize BUFFER_FLUSH_CAP = BUFFER_SIZE * .75f;
    };

    namespace renderer {
        static constexpr bool ENABLE_VALIDATION_LAYERS = true;
        static constexpr u32 FRAME_OVERLAP = 2;
    };

    namespace input {
        
    };

};