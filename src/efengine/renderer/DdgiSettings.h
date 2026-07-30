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

        // Tuneada para la sala de Cornell de TestScene: interior de 8 x 4 x 8 m
        // con el piso en y=0. La grilla cubre x,z en [-3,3] e y en [0.5,3.5]:
        // un metro de margen a las paredes y medio al piso.
        //
        // El margen no es cosmetico. Un probe DENTRO de una pared captura su
        // interior (negro) y lo reparte a sus vecinos por el peso trilineal.
        DdgiGrid grid { glm::vec3(-3.0f, 0.5f, -3.0f),
                        glm::vec3(1.5f, 1.0f, 1.5f),
                        glm::ivec3(5, 4, 5) };

        // -- Update --
        u32  probesPerFrame = 8u;      // se clampea a [0, kMaxProbesPerFrame]
        bool freeze         = false;   // congela el round-robin; el sampleo sigue
        f32  hysteresis     = 0.97f;   // cuanto del valor viejo se conserva

        // -- Sampleo --
        f32 intensity          = 1.0f;
        f32 normalBias         = 0.25f;   // metros
        f32 viewBias           = 0.1f;    // metros
        // Far plane de la captura; NO va al UBO. La diagonal de la sala es 12 m,
        // asi que 15 deja margen sin desperdiciar precision de depth: el valor
        // viejo de 200 m era de la sala de 200 m de sandbox.efe, y en un interior
        // chico tira casi todo el rango del depth buffer a la basura.
        f32 maxDistance        = 15.0f;
        f32 chebyshevSharpness = 3.0f;

        // -- Debug --
        // Ya hay panel (menu Render > DDGI): el debug arranca apagado.
        bool debugProbes = false;
        u32  debugMode   = 0u;      // 0=irradiancia, 1=media de distancia, 2=target de captura
        f32  debugRadius = 0.08f;   // metros; escalado al paso de 0.3 m de la grilla
    };

}
}
