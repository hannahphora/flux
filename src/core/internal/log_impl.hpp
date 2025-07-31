#pragma once
#include <common/log.hpp>

namespace flux::log {
    static char buffer[config::log::BUFFER_SIZE] = {0};
    static usize index = 0;
    static const char* levelStrings[] = {
        "DEBUG",
        "WARNING",
        "ERROR",
        "VULKAN",
    };

    void abortIfError(level lvl) {
        if (lvl == level::ERROR) {
            flush();
            std::abort();
        }
    }
}

void log::buffered(const std::string& msg, level lvl) {
    sprintf(buffer + index, "[%s] %s\n", levelStrings[(usize)lvl], msg.c_str());
    index += strlen(buffer + index);
    abortIfError(lvl);

    if (index >= config::log::BUFFER_FLUSH_CAP)
        flush();
}

void log::unbuffered(const std::string& msg, level lvl) {
    fprintf(config::log::OUTPUT_FILE, "[%s] %s\n", levelStrings[(usize)lvl], msg.c_str());
    abortIfError(lvl);
}

void log::flush() {
    fwrite(buffer, 1, index, config::log::OUTPUT_FILE);
    memset(buffer, 0, index);
    index = 0;
}
