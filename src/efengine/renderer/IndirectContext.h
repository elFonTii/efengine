#pragma once
#include <efengine/core/Types.h>

#include <efengine/renderer/ReducedRes.h>

namespace efengine {
namespace renderer {

    class Texture;

    // Lo que un frame necesita saber de la indirecta difusa resuelta a media
    // resolucion. Espeja a ShadowContext, IblContext, DdgiContext y AoContext:
    // punteros no-duenos al IndirectPass, que vive en Application.
    //
    // Si texture es null, pbr.frag NO cae a negro: vuelve a samplear el volumen
    // inline, que es exactamente lo que hacia antes de que este pase existiera.
    // Esa es la unica razon por la que el camino inline sigue compilado en el
    // shader -- ver el comentario de IndirectPass.h sobre el AO apagado.
    struct IndirectContext {
        const Texture* texture = null;

        // Cuantos texels de resolucion completa cubre uno de `texture` por eje.
        // El upsample lo necesita para saber en que texel del prepass-guia
        // (que esta a resolucion completa) buscar la guia de cada tap.
        i32 scale = kReducedScale;

        // El prepass-guia NO esta aca: vive en AoContext, que es quien lo
        // produce. Ver el comentario de AoContext::depthNormal. La validez de
        // este contexto depende de los dos, asi que la decide MakeAoBlock con
        // los dos en la mano y no este struct por su cuenta.
        bool Valid() const { return texture != null && scale > 0; }
    };

}
}
