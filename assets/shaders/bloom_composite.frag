#version 450 core
in vec2 vUV;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uScene;      // escena HDR full-res
layout(binding = 1) uniform sampler2D uBloom;      // bloom desenfocado (1/2 res; LINEAR upscalea)

layout(std140, binding = 4) uniform PassParams {
    vec4 uParams;   // x = intensidad del halo
};

// Composicion aditiva en HDR lineal, antes del tonemap.
void main() {
    vec3 scene = texture(uScene, vUV).rgb;
    vec3 bloom = texture(uBloom, vUV).rgb;
    FragColor = vec4(scene + uParams.x * bloom, 1.0);
}
