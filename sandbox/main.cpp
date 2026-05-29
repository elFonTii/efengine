#include<efengine/core/Log.h>
#include<efengine/core/Assert.h>
#include<efengine/core/Types.h>

int main() {
    EF_LOG_INFO("=== efengine Sandbox Started ===");

    // Probar logs
    EF_LOG_INFO("This is an info message");
    EF_LOG_WARNING("This is a warning message");
    EF_LOG_DEBUG("This is a debug message");

    // Probar asserts
    int version = 17;
    EF_ASSERT(version == 17, "C++ version should be 17");

    EF_LOG_INFO("All tests passed!");
    EF_LOG_INFO("=== efengine Sandbox Ended ===");

    return 0;
}