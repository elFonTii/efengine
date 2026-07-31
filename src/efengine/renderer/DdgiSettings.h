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
        // El margen no es cosmetico, pero ya no es la unica defensa: un probe
        // DENTRO de una pared NO captura su interior -- con backface culling la
        // cara interna se descarta y el probe ve derecho a traves, hacia el
        // exterior. De ahi salia el light leaking. Lo resuelve la clasificacion
        // por backfaces de abajo; el margen solo reduce cuantos probes la
        // necesitan.
        DdgiGrid grid { glm::vec3(-3.0f, 0.5f, -3.0f),
                        glm::vec3(1.5f, 1.0f, 1.5f),
                        glm::ivec3(5, 4, 5) };

        // -- Update --
        u32  probesPerFrame = 10u;     // se clampea a [0, kMaxProbesPerFrame]
        bool freeze         = false;   // congela el round-robin; el sampleo sigue

        // Cuanto del valor viejo se conserva. 0.4 y no el 0.97 de la referencia:
        // con 256 probes a 10 por frame un barrido tarda 26 frames, asi que 0.97
        // tardaba varios segundos en reaccionar a mover el sol -- que es
        // exactamente lo que uno hace mientras tunea. El precio es mas ruido
        // temporal, aceptable con la grilla y el conteo de esta escena.
        f32  hysteresis     = 0.4f;

        // -- Sampleo --
        f32 intensity          = 1.1f;
        f32 normalBias         = 0.25f;   // metros
        f32 viewBias           = 0.1f;    // metros
        // Far plane de la captura y techo de las distancias en el shader; viaja
        // en params2.x al UBO. Ademas de la proyeccion de captura, lo usan
        // blend_distance.comp para recortar distancias y debug_probe.frag para
        // normalizarlas: antes cada uno tenia su propia constante 4*spacing y
        // el slider del panel no movia ninguna de las dos.
        //
        // 7 m esta DEBAJO de la diagonal de 12 m de la sala, y es a proposito:
        // un probe pegado a una pared no llega a capturar la de enfrente (8 m).
        // Recorta rebote lejano a cambio de concentrar la precision de depth
        // donde el Chebyshev decide, que es cerca. Valor medido en pantalla, no
        // derivado; si aparece luz faltante en las paredes opuestas, este es el
        // primer numero a subir.
        f32 maxDistance        = 7.0f;
        f32 chebyshevSharpness = 3.6f;

        // -- Clasificacion de probes --
        // Fraccion de texels de backface a partir de la cual un probe empieza a
        // perder peso, y a partir de la cual lo pierde del todo. Son DOS y no uno
        // porque el sampleo usa un smoothstep: un corte binario hace pop cuando un
        // objeto se mueve y cruza el umbral. La referencia RTXGI marca inactivo a
        // partir del 25%, y este par lo deja adentro de la transicion.
        f32 backfaceFadeStart = 0.15f;
        f32 backfaceFadeEnd   = 0.30f;

        // -- Debug --

        // Que termino del shading escribe pbr.frag en vez de la imagen final.
        //
        // NO es cosmetico. DDGI entra al pixel como kD * irradiancia * albedo *
        // ao, adentro de `ambient`, y `ambient` se suma al sol en la misma
        // linea. En la imagen final "DDGI aporta cero" y "DDGI aporta poco" son
        // el mismo pixel: no hay forma de distinguirlos mirando. Este enum es la
        // que hay.
        //
        // Viaja en params1.z del bloque DDGI y no en FrameBlock porque extender
        // Frame obliga a tocar los siete shaders que lo declaran (ver el
        // comentario de DdgiBlock en ShaderBlocks.h) sin que ninguno lo use.
        // Los valores tienen que coincidir con las constantes kDdgiView* de
        // ddgi/common.glsl y con el orden del combo en EditorUI.cpp.
        enum DebugView : u32 {
            kDebugOff             = 0u,   // imagen final
            kDebugIndirect        = 1u,   // irradiancia indirecta cruda (DDGI o IBL)
            kDebugIndirectApplied = 2u,   // lo que esa irradiancia le suma al pixel
            kDebugDirect          = 3u,   // solo luz directa (sol + puntuales)
            kDebugDdgiNoFade      = 4u,   // DDGI ignorando DdgiVolumeFade
            kDebugFade            = 5u,   // el propio fade, en gris
            kDebugAlbedo          = 6u,
            kDebugNormal          = 7u,
        };
        u32 debugView = kDebugOff;

        // Ya hay panel (menu Render > DDGI): el debug arranca apagado.
        bool debugProbes = false;
        u32  debugMode   = 0u;      // 0=irradiancia, 1=media de distancia, 2=target de captura
        f32  debugRadius = 0.08f;   // metros; escalado al paso de 0.3 m de la grilla
    };

}
}
