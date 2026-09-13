#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    class Cubemap;
    class Texture;

    // Lo que un frame necesita saber del IBL ya precomputado. Espeja a ShadowContext:
    // punteros no-dueños al Environment, que vive en Application.
    // Si falta cualquiera de los tres mapas, el shader apaga el ambiente entero
    // (uHasIbl = 0) en vez de muestrear una unidad de textura equivocada.
    struct IblContext {
        // El cubemap CRUDO del entorno, no el irradiance ni el prefiltrado. Lo
        // consumen SkyboxPass, para dibujar el cielo, y la captura de DDGI.
        // Viaja aca y no se le pide al IblPass directamente: SceneLighting es
        // la unica via por la que un pase alcanza lo que produjo otro.
        const Cubemap* environment = null;
        const Cubemap* irradiance  = null;
        const Cubemap* prefiltered = null;
        const Texture* brdfLut     = null;

        f32 maxLod    = 0.0f;   // Environment::prefilterMaxLod()
        f32 intensity = 1.0f;   // SceneGraph::iblIntensity
    };

}
}
