#include "Log.h"
#include <cstdarg>
#include <iostream>
#include <ctime>

namespace efengine {
namespace core {
    static const char* GetLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARNING";
            case LogLevel::Error: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    void Log(LogLevel level, const char* format, ...) {
        const char* level_str = GetLevelString(level);

        char buffer[512];

        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        std::printf("%s%s\n ", level_str, buffer);
        std::fflush(stdout);  // Hace que aparezca inmediatamente
    }
}
}
