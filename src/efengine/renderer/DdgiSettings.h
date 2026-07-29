#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/DdgiVolume.h>

namespace efengine {
namespace renderer {

    // Todo lo ajustable de DDGI, y exactamente lo que expone el panel de ImGui.
    // Espeja el rol de ShadowSettings, pero vive en su propio header porque
    // MakeDdgiBlock (en ShaderBlocks.cpp) lo lee y no puede depender de DdgiPass.
    //
    // Nada de esto esta serializado: la grilla se tunea por ImGui y se pierde al
    // cerrar. Llevarlo al .efe es el ciclo de v4.
    struct DdgiSettings {
        bool enabled = true;

        DdgiGrid grid;   // origin, spacing, counts

        // -- Update --
        u32  probesPerFrame = 8u;      // se clampea a [0, kMaxProbesPerFrame]
        bool freeze         = false;   // congela el round-robin; el sampleo sigue
        f32  hysteresis     = 0.97f;   // cuanto del valor viejo se conserva

        // -- Sampleo --
        f32 intensity          = 1.0f;
        f32 normalBias         = 0.25f;   // metros
        f32 viewBias           = 0.1f;    // metros
        f32 maxDistance        = 8.0f;    // far plane de la captura; NO va al UBO
        f32 chebyshevSharpness = 3.0f;

        // -- Debug --
        // TEMPORAL hasta la tarea 15 (panel de ImGui): el volcado del target de
        // captura arranca prendido porque es la unica forma de verificar la
        // captura mientras no hay UI que lo encienda.
        bool debugProbes = true;
        u32  debugMode   = 2u;      // 0=irradiancia, 1=media de distancia, 2=target de captura
        f32  debugRadius = 0.12f;   // metros
    };

}
}
