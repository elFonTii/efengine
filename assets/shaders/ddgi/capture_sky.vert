#version 450 core
// El cielo de la captura, instanciado por (probe, cara) igual que la geometria.
//
// Reemplaza el uso de skybox.vert que hacia este pase: la matriz que reconstruye
// la direccion de vista ya no viene del bloque Frame sino del tile de la
// instancia.
//
// El quad cubre exactamente [-1,1], asi que despues de la escala cubre
// exactamente su rectangulo del atlas y no hace falta recortarlo. Por eso
// DdgiPass apaga los planos de recorte para este draw: dejarlos habilitados
// pondria los cuatro planos JUSTO sobre los bordes del quad, y una distancia de
// exactamente cero es la peor entrada posible para un test de recorte.
layout(location = 0) in vec2 aPos;   // esquina del quad, en NDC [-1,1]

#include "ddgi/capture_tiles.glsl"

out vec3 vDir;                        // direccion de vista en espacio mundo

void main() {
    DdgiCaptureTile tile = uTiles[gl_InstanceID];

    vec4 world = tile.invViewProjRot * vec4(aPos, 1.0, 1.0);  // punto en el plano lejano
    vDir = world.xyz / world.w;

    // z = w -> profundidad lejana. La escala al tile no toca zw.
    gl_Position = DdgiTileClip(vec4(aPos, 1.0, 1.0), tile.rect);
}
