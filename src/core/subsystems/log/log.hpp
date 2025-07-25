#pragma once
#include <common/common.hpp>
#include <common/config.hpp>

namespace flux::log {
//--------------------------------------------------------------------------------------------

enum class level : u8 {
    DEBUG,
    WARNING,
    ERROR,
};

void buffered(const std::string& msg, level lvl = level::DEBUG);
void unbuffered(const std::string& msg, level lvl = level::DEBUG);
void flush();

//--------------------------------------------------------------------------------------------
}