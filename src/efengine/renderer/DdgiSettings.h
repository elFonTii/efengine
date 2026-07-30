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

        // Tuneada para la escena de prueba actual: un cubo de 2x2x2 m. 6 probes
        // por eje cada 0.3 m cubren 1.5 m centrados en el interior, sin pegar los
        // probes a las paredes (un probe DENTRO de una pared captura su interior
        // y contamina a sus vecinos por el peso trilineal).
        //
        // Los defaults de DdgiGrid (origen -6, paso 1.5) son de un interior de
        // ~12 m; el tuning definitivo es la tarea 13.
        DdgiGrid grid { glm::vec3(-0.75f, 0.25f, -0.75f),
                        glm::vec3(0.3f),
                        glm::ivec3(6, 6, 6) };

        // -- Update --
        u32  probesPerFrame = 8u;      // se clampea a [0, kMaxProbesPerFrame]
        bool freeze         = false;   // congela el round-robin; el sampleo sigue
        f32  hysteresis     = 0.97f;   // cuanto del valor viejo se conserva

        // -- Sampleo --
        f32 intensity          = 1.0f;
        f32 normalBias         = 0.25f;   // metros
        f32 viewBias           = 0.1f;    // metros
        // Far plane de la captura; NO va al UBO. El plan proponia 8 m, tuneado
        // para un interior chico: con eso la captura sale VACIA en la escena del
        // sandbox, cuya sala mide 200 m de lado y tiene las paredes a ~94 m de
        // los probes. Todo lo que este mas lejos que esto no existe para la GI.
        f32 maxDistance        = 200.0f;
        f32 chebyshevSharpness = 3.0f;

        // -- Debug --
        // TEMPORAL hasta la tarea 15 (panel de ImGui): el debug arranca prendido
        // porque es la unica forma de verificar el pase mientras no hay UI que lo
        // encienda. Modo 1 para verificar los momentos de distancia (tarea 12);
        // el 0 (irradiancia, tarea 11) y el 2 (target de captura, tarea 9)
        // siguen disponibles.
        bool debugProbes = true;
        u32  debugMode   = 1u;      // 0=irradiancia, 1=media de distancia, 2=target de captura
        f32  debugRadius = 0.04f;   // metros; escalado al paso de 0.3 m de la grilla
    };

}
}
