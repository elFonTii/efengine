#pragma once

#include "Log.h"

// 1. Namespace
namespace efengine {
namespace core {

// 2. Variables

// 3. Funciones
void AssertFailed(const char* condition, const char* file, int line, const char* message);
    

}
}

// 4. Macros (Condicionales para debug/release)
#ifdef _DEBUG
    #define EF_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                ::efengine::core::AssertFailed(#condition, __FILE__, __LINE__, message); \
            } \
        } while (false)
#else
    #define EF_ASSERT(condition, message)((void)0) // No hace nada en release
#endif

#define EF_ASSERT_MSG(condition, message) EF_ASSERT(condition, message)