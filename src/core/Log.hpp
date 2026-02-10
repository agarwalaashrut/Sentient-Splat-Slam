#pragma once

#include <cstdio>
#include <string>

namespace sss {

class Log {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };

    static void init();
    
    static void debug(const char* fmt, ...);
    static void info(const char* fmt, ...);
    static void warning(const char* fmt, ...);
    static void error(const char* fmt, ...);

private:
    static void log(Level level, const char* fmt, va_list args);
    static const char* level_to_string(Level level);
};

}  // namespace sss

// Convenience macros
#define SSS_LOG_DEBUG(...) sss::Log::debug(__VA_ARGS__)
#define SSS_LOG_INFO(...) sss::Log::info(__VA_ARGS__)
#define SSS_LOG_WARN(...) sss::Log::warning(__VA_ARGS__)
#define SSS_LOG_ERROR(...) sss::Log::error(__VA_ARGS__)
