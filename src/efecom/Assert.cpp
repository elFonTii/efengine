// Assert.cpp
//  Implementación del assert de efecom: reporta a stderr y aborta.
//  No usa el Log de efengine (el RHI no puede depender del motor).
#include "Assert.h"

#include <cstdio>
#include <cstdlib>

namespace efecom {

    void AssertFailed(const char* condition, const char* file, int line, const char* message) {
        std::fprintf(stderr, "[EFCOM ASSERT] %s\n  condicion: %s\n  en: %s:%d\n",
                     message, condition, file, line);
        std::abort();
    }

}
