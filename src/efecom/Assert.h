// Assert.h
//  Assert propio de efecom (el RHI no puede depender de efengine/core).
//  Mismo contrato que EF_ASSERT: activo con _DEBUG, no-op en release.
#pragma once

namespace efecom {

    void AssertFailed(const char* condition, const char* file, int line, const char* message);

}

#ifdef _DEBUG
    #define EFCOM_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                ::efecom::AssertFailed(#condition, __FILE__, __LINE__, message); \
            } \
        } while (false)
#else
    #define EFCOM_ASSERT(condition, message)((void)0)
#endif
