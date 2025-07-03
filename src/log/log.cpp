#include <log/log.h>

static char logBuffer[config::log::BUFFER_SIZE] = {0};
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
    fprintf(config::log::OUTPUT_FILE, "[%s] %s\n", LogLevelStrings[(usize)lvl], msg);
}

void log::flush() {
    fwrite(logBuffer, sizeof(char), logBufferIndex, config::log::OUTPUT_FILE);
    memset(logBuffer, 0, logBufferIndex);
    logBufferIndex = 0;
}
