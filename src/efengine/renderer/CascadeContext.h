#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/ShadowMath.h>

namespace efengine {
namespace renderer {

    class CascadedShadowMap;

    // Lo que el CascadedShadowPass publica en SceneLighting para que el Renderer
    // lo suba. Puntero no-dueno, igual que ShadowContext::map.
    struct CascadeContext {
        const CascadedShadowMap* map     = null;
        bool                     enabled = false;
        u32                      count   = 0u;

        // Fraccion final de cada cascada que se mezcla con la siguiente.
        f32 blendRatio = 0.1f;

        // Viaja aca y no como parametro suelto para que el Renderer no tenga que
        // conocer al pase ni a sus settings.
        f32 normalOffsetTexels = 2.0f;

        bool debugView = false;

        CascadeFit fits[kMaxCascades] {};
    };

}
}
