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
        DdgiGrid grid { glm::vec3(-21.6f, -27.1f, -43.6f),
                        glm::vec3(5.35f, 4.55f, 6.0f),
                        glm::ivec3(10, 16, 18) };

        // -- Update --
        u32  probeBudget           = 128u;               // tope por frame, se clampea a kMaxProbesPerFrame
        u32  raysPerProbe          = kMaxRaysPerProbe;   // [kMinRaysPerProbe, kMaxRaysPerProbe]
        u32  inactiveRecheckSweeps = 8u;                 // una inactiva se revisa cada tantos barridos
        bool freeze                = false;              // congela la actualizacion (no corre la planificacion); el sampleo sigue

        // Tope de la histeresis (RTXGI: 0,97). Cada probe arranca en 0 despues de un reset y
        // sube como promedio progresivo (n-1)/n hasta este valor: el tope decide cuanto
        // ruido se filtra en regimen, no cuanto tarda en arrancar.
        f32  hysteresis            = 0.97f;

        // Deteccion de cambio grande (RTXGI), en el blend de irradiancia.
        f32  irradianceThreshold   = 0.2f;   // espacio perceptual, x^(1/5)
        f32  brightnessThreshold   = 2.0f;   // lineal

        // Delta media perceptual por debajo de la cual el panel da el volumen por
        // convergido. Medido en docs/superpowers/sims/2026-10-01-ddgi_rayos_sim.mjs: con 256
        // rayos se cruza a las ~8 actualizaciones, con error RMS ~0,24 %.
        f32  convergenceEpsilon    = 6.0e-4f;

        // -- Sampleo --
        f32 intensity          = 1.1f;
        f32 normalBias         = 0.25f;   // metros
        f32 viewBias           = 0.1f;    // metros
        f32 chebyshevSharpness = 3.6f;

        // -- Trazado contra voxeles --
        // Alfa minimo de un voxel para que el DDA lo cuente como solido. El 0.5
        // es EXPLICITO y no un cero por omision: con umbral 0 la condicion
        // alpha >= umbral es cierta en el aire, el rayo muere en el primer
        // voxel del grid y la GI se arruina sin que nada falle ruidosamente.
        f32 opacityThreshold = 0.5f;

        // -- Clasificacion de probes --
        // Fraccion de texels de backface a partir de la cual un probe empieza a
        // perder peso, y a partir de la cual lo pierde del todo. Son DOS y no uno
        // porque el sampleo usa un smoothstep: un corte binario hace pop cuando un
        // objeto se mueve y cruza el umbral. La referencia RTXGI marca inactivo a
        // partir del 25%, y este par lo deja adentro de la transicion.
        f32 backfaceFadeStart = 0.15f;
        f32 backfaceFadeEnd   = 0.30f;

        // Aplica la fraccion de arriba como peso en el sampleo. Apagado sirve para comparar.
        bool classificationEnabled = true;

        // -- Reubicacion de probes --
        // probe_update.comp mueve cada probe con las reglas de RTXGI, hasta el 45%
        // del espaciado. Apagado escribe offset 0.
        bool relocationEnabled = true;

        // Metros. Un probe mas cerca que esto de una cara delantera se aleja; uno
        // adentro de algo queda a la mitad de esto del otro lado. Escala de Bistro.
        f32 minFrontfaceDistance = 1.0f;

        // -- Diagnostico --

        // Ablation test del pase Forward: pbr.frag devuelve una irradiancia
        // CONSTANTE en vez de samplear el volumen, y no toca nada mas del
        // shading. Es un instrumento de medicion, no un modo de imagen.
        //
        // Existe porque el Forward cuesta ~9.300 ciclos por pixel con 6 draws,
        // que es 10-20x lo que explica su aritmetica: por descarte tiene que
        // ser latencia de memoria, y el sospechoso es el sampleo del volumen
        // (8 probes trilineales x 2 lookups octaedricos = ~16 gathers con
        // coordenadas calculadas por pixel, sin prefetch posible).
        //
        // Como se lee: con esto en true el Forward tiene que caer a ~0.3 ms. Si
        // cae, el cuello es el sampleo y la solucion es resolverlo a media
        // resolucion (IndirectPass). Si no cae, el cuello esta en otro lado y
        // hay que mirar ocupancia en Nsight antes de tocar nada.
        //
        // NO afecta a la captura de probes: ahi el sampleo es el rebote que
        // realimenta el atlas, y apagarlo cambiaria la imagen de verdad en vez
        // de medir el Forward.
        bool ablateSample = false;

        // El valor que devuelve el ablation. Un gris medio y no negro: con cero
        // el compilador puede plegar la multiplicacion por albedo y borrar
        // trabajo que en el camino real si se hace, y la medicion mentiria.
        f32 ablateIrradiance = 0.25f;

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
            kDebugShadow          = 8u,   // termino de sombra del sol, crudo
        };
        u32 debugView = kDebugOff;

        // Ya hay panel (menu Render > DDGI): el debug arranca apagado.
        bool debugProbes = false;
        u32  debugMode   = 0u;      // 0=irradiancia, 1=media de distancia, 2=target de captura
        f32  debugRadius = 0.08f;   // metros; escalado al paso de 0.3 m de la grilla
    };

}
}
