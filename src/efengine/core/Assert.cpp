#include "Assert.h"
#include <cstdlib>

namespace efengine {
namespace core {

    void AssertFailed(const char* condition, const char* file, int line, const char* message) {
        EF_LOG_ERROR("ASSERT FAILED at{}:{}", file, line);
        EF_LOG_ERROR("  Condition:{}", condition);
        EF_LOG_ERROR("  Message:{}", message);

        #ifdef _DEBUG
            __debugbreak(); // Pausar el debugger en el punto de fallo (MSVC)
        #endif

        // En release, termina el programa.
        std::abort();
    }

}
}