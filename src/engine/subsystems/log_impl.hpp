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
        "TODO",
    };

    inline void abortIfError(level lvl) {
        if (lvl == level::ERROR) {
            flush();
            std::abort();
        }
    }
}

inline void log::buffered(const std::string& msg, level lvl) {
    sprintf(buffer + index, "[%s] %s\n", levelStrings[(usize)lvl], msg.c_str());
    index += strlen(buffer + index);
    abortIfError(lvl);

    if (index >= config::log::BUFFER_FLUSH_CAP)
        flush();
}

inline void log::unbuffered(const std::string& msg, level lvl) {
    fprintf(config::log::OUTPUT_FILE, "[%s] %s\n", levelStrings[(usize)lvl], msg.c_str());
    abortIfError(lvl);
}

inline void log::flush() {
    fwrite(buffer, 1, index, config::log::OUTPUT_FILE);
    memset(buffer, 0, index);
    index = 0;
}

inline void log::debug(const std::string& msg) {
    unbuffered(msg, level::DEBUG);
}

inline void log::warn(const std::string& msg) {
    unbuffered(msg, level::WARNING);
}

inline void log::error(const std::string& msg) {
    unbuffered(msg, level::ERROR);
}

inline void log::todo(const std::string& msg) {
    unbuffered(msg, level::TODO);
}
