#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    class Texture;

    // Lo que un frame necesita saber del AO. Espeja a ShadowContext, IblContext
    // y DdgiContext: puntero no-dueno al AoPass, que vive en Application.
    //
    // Si texture es null, MakeAoBlock apaga params.x y pbr.frag cae al ao del
    // material en vez de samplear una unidad de textura sin contenido.
    //
    // No lleva un "strength": el unico control de fuerza es
    // AoSettings::intensity, aplicado como exponente en gtao.frag. Un segundo
    // exponente aca seria el mismo control dos veces.
    struct AoContext {
        const Texture* texture = null;
        bool enabled     = false;
        bool bentNormal  = false;
        bool multiBounce = false;
        u32  debugView   = 0u;
    };

}
}
