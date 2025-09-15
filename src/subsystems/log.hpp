#pragma once
#include <common.hpp>
#include <config.hpp>

namespace flux::log {

    enum class level : u8 {
        DEBUG,
        WARNING,
        ERROR,
        VULKAN,
        TODO,
    };

    void buffered(const std::string& msg, level lvl = level::DEBUG);
    void unbuffered(const std::string& msg, level lvl = level::DEBUG);
    void flush();

    void debug(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void todo(const std::string& msg);

}