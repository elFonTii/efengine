// Sistema de logging inicial para el motor.
#pragma once

#include <string>
#include <cstdio>

namespace efengine {
namespace core {

enum class LogLevel { // Niveles de log
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
};

// Función de principal
void Log(LogLevel level, const char* format, ...);

// Macros
#define EF_LOG_DEBUG(fmt, ...)\
::efengine::core::Log(::efengine::core::LogLevel::Debug, fmt, ##__VA_ARGS__)

#define EF_LOG_INFO(fmt, ...)\
::efengine::core::Log(::efengine::core::LogLevel::Info, fmt, ##__VA_ARGS__)

#define EF_LOG_WARNING(fmt, ...)\
::efengine::core::Log(::efengine::core::LogLevel::Warning, fmt, ##__VA_ARGS__)

#define EF_LOG_ERROR(fmt, ...)\
::efengine::core::Log(::efengine::core::LogLevel::Error, fmt, ##__VA_ARGS__)

}
}