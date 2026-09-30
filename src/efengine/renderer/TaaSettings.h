#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    struct TaaSettings {
        f32  alpha         = 0.1f;   // peso del frame actual
        bool debugVelocity = false;
    };

    // Por debajo, un cambio real de la escena tarda cientos de frames en entrar.
    constexpr f32 kTaaMinAlpha = 0.02f;

}
}
