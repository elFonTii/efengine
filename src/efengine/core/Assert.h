#pragma once

#include "Log.h"

namespace efengine {
namespace core {

void AssertFailed(const char* condition, const char* file, int line, const char* message);

}
}

#ifdef _DEBUG
    #define EF_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                ::efengine::core::AssertFailed(#condition, __FILE__, __LINE__, message); \
            } \
        } while (false)
#else
    #define EF_ASSERT(condition, message)((void)0)
#endif

#define EF_ASSERT_MSG(condition, message) EF_ASSERT(condition, message)

// Hay dos clases distintas de chequeo y meterlas en la misma macro fue un error:
//
//   EF_ASSERT     - el programador prometio algo (un puntero no nulo, un indice
//                   en rango). Compilarlo fuera en release es correcto.
//   EF_GPU_CHECK  - el DRIVER contesto algo (un FBO completo, un buffer que
//                   entra). Es una condicion de runtime que depende del driver
//                   y de los assets, y tiene que sobrevivir a release aunque sea
//                   como log de error.
//
// En debug aborta igual que un assert, para parar en el sitio del problema.
#ifdef _DEBUG
    #define EF_GPU_CHECK(condition, message) EF_ASSERT(condition, message)
#else
    #define EF_GPU_CHECK(condition, message) \
        do { \
            if (!(condition)) { \
                EF_LOG_ERROR("[GPU] %s (%s:%d)", message, __FILE__, __LINE__); \
            } \
        } while (false)
#endif