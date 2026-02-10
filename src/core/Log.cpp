#include <src/core/Log.hpp>
#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace sss {

void Log::init() {
    // Could add file logging or other setup here
}

void Log::debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(Level::Debug, fmt, args);
    va_end(args);
}

void Log::info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(Level::Info, fmt, args);
    va_end(args);
}

void Log::warning(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(Level::Warning, fmt, args);
    va_end(args);
}

void Log::error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(Level::Error, fmt, args);
    va_end(args);
}

void Log::log(Level level, const char* fmt, va_list args) {
    std::fprintf(stderr, "[%s] ", level_to_string(level));
    std::vfprintf(stderr, fmt, args);
    std::fprintf(stderr, "\n");
}

const char* Log::level_to_string(Level level) {
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error:   return "ERROR";
        default:             return "UNKNOWN";
    }
}

}  // namespace sss
