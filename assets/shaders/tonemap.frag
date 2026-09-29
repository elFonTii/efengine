#version 450 core
// HDR lineal -> LDR sRGB. Lo que antes hacia el final de bloom_composite.
in vec2 vUV;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uScene;

layout(std140, binding = 4) uniform PassParams {
    vec4 uParams;   // x = exposicion lineal (1.0 = neutra)
};

#include "common/tonemap.glsl"

void main() {
    FragColor = vec4(TonemapToSrgb(texture(uScene, vUV).rgb, uParams.x), 1.0);
}
