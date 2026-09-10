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

        // El prepass de profundidad + normal, SIEMPRE a resolucion completa.
        //
        // Viaja aca y no en IndirectContext aunque lo consuman los dos: lo
        // produce el AoPass, y es la GUIA de los dos upsamples de pbr.frag (el
        // del propio AO y el de la indirecta difusa). Sin el, ninguno de los dos
        // es posible -- los pesos no tienen contra que comparar.
        const Texture* depthNormal = null;

        // Cuantos texels de resolucion completa cubre uno de `texture` por eje.
        // 1 = el AO esta a resolucion completa y pbr.frag lo lee con un
        // texelFetch directo; > 1 = hay que subirlo con el filtro bilateral.
        //
        // Es independiente de que la indirecta este o no resuelta aparte: el AO
        // puede estar a media resolucion con el IndirectPass apagado, y leerlo
        // ahi con coordenadas de resolucion completa devolveria solo el cuadrante
        // superior izquierdo estirado sobre toda la pantalla.
        i32 scale = 1;

        bool enabled     = false;
        bool bentNormal  = false;
        bool multiBounce = false;
        u32  debugView   = 0u;
    };

}
}
