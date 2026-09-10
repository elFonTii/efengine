#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    // LA regla de resolucion reducida del motor. La comparten AoPass (la
    // visibilidad y el bent normal del GTAO) e IndirectPass (la irradiancia
    // difusa de DDGI), y tiene que ser exactamente la misma en los dos: si sus
    // targets no salen del mismo tamano, indirect.frag lee el bent normal del
    // AO en una grilla que no le corresponde y devuelve el de otro pixel.
    //
    // Ese bug no rompe nada ruidosamente -- solo tuerce la direccion del color
    // bleeding --, asi que la unica defensa razonable es que no haya dos
    // formulas que puedan divergir.

    // Cuantos texels de resolucion completa cubre uno del target por eje.
    //
    // 2 y no 4: a un cuarto por eje (1/16 de los pixeles) el upsample bilateral
    // se queda sin taps validos demasiado seguido. Un objeto de menos de 4 px de
    // ancho no tiene NINGUN texel propio en el target reducido, y el fallback al
    // tap mas parecido deja de ser un escalon de un pixel para pasar a ser el
    // color de otra superficie.
    inline constexpr i32 kReducedScale = 2;

    // Redondea HACIA ARRIBA. Con un alto impar (1079), un floor dejaria la
    // ultima fila de pixeles sin ningun texel que la cubra y el upsample la
    // resolveria con el clamp del borde: una franja de un pixel con el valor de
    // la fila de arriba, justo en el borde de la pantalla.
    inline u32 ReducedExtent(u32 full) {
        const u32 s = static_cast<u32>(kReducedScale);
        return (full + s - 1u) / s;
    }

}
}
