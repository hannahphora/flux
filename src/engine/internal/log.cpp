#pragma once
#include "../log.hpp"

namespace flux::log {
    static char buffer[config::log::BUFFER_SIZE] = {0};
    static usize index = 0;
    static const char* levelStrings[] = {
        "DEBUG",
        "WARNING",
        "ERROR",
        "VULKAN",
        "TODO",
    };
}

void log::buffered(const std::string& msg, level lvl) {
    sprintf(buffer + index, "[%s] %s\n", levelStrings[(usize)lvl], msg.c_str());
    index += strlen(buffer + index);

    if (index >= config::log::BUFFER_FLUSH_CAPACITY)
        flush();
}

void log::unbuffered(const std::string& msg, level lvl) {
    fprintf(config::log::OUTPUT_FILE, "[%s] %s\n", levelStrings[(usize)lvl], msg.c_str());
}

void log::flush() {
    fwrite(buffer, 1, index, config::log::OUTPUT_FILE);
    memset(buffer, 0, index);
    index = 0;
}

void log::debug(const std::string& msg) {
    unbuffered(msg, level::DEBUG);
}

void log::warn(const std::string& msg) {
    unbuffered(msg, level::WARNING);
}

void log::error(const std::string& msg) {
    unbuffered(msg, level::ERROR);
}

void log::todo(const std::string& msg) {
    unbuffered(msg, level::TODO);
}
