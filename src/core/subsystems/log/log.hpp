#pragma once
#include <common/types.hpp>
#include <common/config.hpp>

#include <cstdlib>
#include <stdio.h>
#include <cstring>

namespace flux {
//--------------------------------------------------------------------------------------------

enum class LogLevel : u8 {
    DEBUG,
    WARNING,
    ERROR,
    DEFAULT = DEBUG,
};

namespace log {
    void buffered(const char* msg, LogLevel lvl = LogLevel::DEFAULT);
    void unbuffered(const char* msg, LogLevel lvl = LogLevel::DEFAULT);
    void flush();
};

//--------------------------------------------------------------------------------------------
};