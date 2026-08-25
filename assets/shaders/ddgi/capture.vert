#version 450 core
// Vertex shader de la captura de probes, INSTANCIADO por (probe, cara).
//
// Reemplaza el uso de pbr.vert que hacia este pase. Dos motivos, y el segundo
// solo no habria alcanzado para justificarlo:
//
//  1. La vista ya no viene del bloque Frame (una por draw) sino del SSBO de
//     tiles, indexado por gl_InstanceID. Es lo que permite dibujar las 6 caras
//     de todos los probes del frame con un solo draw por objeto.
//  2. capture.frag es difuso puro: usa la normal geometrica y nada mas. La TBN
//     completa de pbr.vert -- dos normalize, una transpose(inverse), un cross y
//     un Gram-Schmidt por vertice -- se calculaba entera para tirar dos de sus
//     tres columnas.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

layout(std140, binding = 2) uniform Object {
    mat4 uModel;
};

#include "ddgi/capture_tiles.glsl"

out vec3 vFragPos;
out vec2 vUV;
out vec3 vNormal;
// El centro del probe deja de venir por el bloque Frame: con un solo draw para
// todas las vistas, cada instancia tiene el suyo. flat porque es constante en
// toda la primitiva -- interpolarlo seria calcular un numero que ya se sabe.
flat out vec3 vProbeCenter;

out float gl_ClipDistance[4];

void main() {
    DdgiCaptureTile tile = uTiles[gl_InstanceID];

    vec4 world = uModel * vec4(aPos, 1.0);
    vFragPos = world.xyz;
    vUV      = aUV;

    // La inversa transpuesta para que la escala no uniforme no tuerza la normal.
    vNormal = mat3(transpose(inverse(mat3(uModel)))) * aNormal;

    vProbeCenter = tile.probeCenter.xyz;

    vec4 clip = tile.viewProj * world;
    DdgiWriteTileClip(clip, gl_ClipDistance[0], gl_ClipDistance[1],
                            gl_ClipDistance[2], gl_ClipDistance[3]);

    gl_Position = DdgiTileClip(clip, tile.rect);
}
