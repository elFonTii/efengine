#version 450 core
// Composite del bloom Y tone mapping, en un solo pase.
//
// Los dos estaban separados y el tonemap era un pase fullscreen dedicado: leer
// 1920x1080 de RGBA16F, aplicar una curva de ~10 flops por pixel y escribir
// otro tanto. A 448 GB/s eso son ~0.04 ms de puro trafico de memoria para hacer
// diez operaciones por pixel. Fusionado, la curva se aplica sobre un valor que
// este shader YA tiene en registros y desaparecen una lectura y una escritura de
// pantalla completa.
//
// El orden se mantiene: composite -> tonemap -> FXAA. FXAA tiene que correr
// sobre la imagen LDR ya tonemapeada -- su estimador de contraste asume valores
// perceptuales, y sobre HDR lineal los highlights le parecen bordes.
//
// La suma del bloom sigue siendo en HDR LINEAL, antes de la curva: sumar dos
// imagenes ya tonemapeadas no es sumar luz, y el halo saldria apagado justo
// donde mas brilla.
in vec2 vUV;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uScene;      // escena HDR full-res
layout(binding = 1) uniform sampler2D uBloom;      // bloom desenfocado (1/2 res; LINEAR upscalea)

layout(std140, binding = 4) uniform PassParams {
    vec4 uParams;   // x = intensidad del halo, y = exposicion lineal (1.0 = neutra)
};

#include "common/tonemap.glsl"

void main() {
    vec3 scene = texture(uScene, vUV).rgb;
    vec3 bloom = texture(uBloom, vUV).rgb;

    vec3 hdr = scene + uParams.x * bloom;

    FragColor = vec4(TonemapToSrgb(hdr, uParams.y), 1.0);
}
