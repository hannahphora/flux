#pragma once
#include <common.hpp>

namespace flux::config {

    static const std::string ENGINE_NAME = "flux";
    static const std::string APP_NAME = "TestApplication";

    static constexpr usize WINDOW_WIDTH = 800;
    static constexpr usize WINDOW_HEIGHT = 600;

    namespace log {
        static FILE* OUTPUT_FILE = stderr;
        static constexpr usize BUFFER_SIZE = 8 * 1024 * 1024; // 8mb
        static constexpr usize BUFFER_FLUSH_CAPACITY = usize(BUFFER_SIZE * .75f);
    }
}