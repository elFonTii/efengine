#pragma once
#include <efengine/core/Types.h>

#include <glm/glm.hpp>

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

        // El prepass de profundidad + normal A RESOLUCION COMPLETA, que es la
        // GUIA del upsample bilateral. Sale del AoPass; viaja aca y no en
        // AoContext porque quien lo consume es el upsample, no el AO.
        //
        // Sin el no hay upsample posible: los pesos no tienen contra que
        // comparar. Si falta, pbr.frag vuelve al camino inline igual que si
        // faltara `texture`.
        const Texture* depthNormal = null;

        // Resolucion en texels de `texture`. La necesita el clamp de taps del
        // upsample: sin ella, el quad 2x2 del borde derecho de la pantalla lee
        // fuera del target.
        glm::ivec2 size { 0, 0 };

        // Cuantos texels de resolucion completa cubre uno de `texture` por eje.
        // Hoy siempre 2 (media resolucion); el upsample esta escrito contra este
        // numero para que bajar a un cuarto sea un cambio de una linea.
        i32 scale = 2;

        // El AO tambien esta a resolucion reducida y hay que subirlo con el
        // mismo filtro. Lo enciende la tarea 2; con el en false pbr.frag lee el
        // AO con texelFetch directo, como siempre.
        bool aoReduced = false;

        bool Valid() const { return texture != null && depthNormal != null
                                 && size.x > 0 && size.y > 0; }
    };

}
}
