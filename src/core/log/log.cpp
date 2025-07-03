#include <core/log/log.h>

#include <iostream>
#include <format>

static FILE* LOG_OUTPUT = stdout;
static constexpr usize LOG_BUFFER_SIZE = 8 * 1024;
static char logBuffer[LOG_BUFFER_SIZE] = {0};
static usize logBufferIndex = 0;

static const char* LogLevelStrings[] = {
    "VERBOSE",
    "INFO",
    "WARNING",
    "ERROR",
};

void log::buffered(const char* msg, LogLevel lvl) {
    sprintf(logBuffer + logBufferIndex, "[%s] %s\n", LogLevelStrings[(usize)lvl], msg);
    logBufferIndex += strlen(logBuffer + logBufferIndex);
}

void log::unbuffered(const char* msg, LogLevel lvl) {
    fprintf(LOG_OUTPUT, "[%s] %s\n", LogLevelStrings[(usize)lvl], msg);
}

void log::flush() {
    fwrite(logBuffer, sizeof(char), logBufferIndex, LOG_OUTPUT);
    memset(logBuffer, 0, logBufferIndex);
    logBufferIndex = 0;
}
