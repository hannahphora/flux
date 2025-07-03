#pragma once
#include <common.h>

namespace flux {
//--------------------------------------------------------------------------------------------

enum class LogLevel : u8 {
    VERBOSE,
    INFO,
    WARNING,
    ERROR,
    DEFAULT = VERBOSE,
};

namespace log {
    void buffered(const char* msg, LogLevel lvl = LogLevel::DEFAULT);
    void unbuffered(const char* msg, LogLevel lvl = LogLevel::DEFAULT);
    void flush();
};

//--------------------------------------------------------------------------------------------
};