#version 450 core
// Composite del bloom en HDR lineal. El tonemap va en su propio pase despues:
// sumar dos imagenes ya tonemapeadas no es sumar luz.
in vec2 vUV;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uScene;      // escena HDR full-res
layout(binding = 1) uniform sampler2D uBloom;      // bloom desenfocado (1/2 res; LINEAR upscalea)

layout(std140, binding = 4) uniform PassParams {
    vec4 uParams;   // x = intensidad del halo
};

void main() {
    vec3 scene = texture(uScene, vUV).rgb;
    vec3 bloom = texture(uBloom, vUV).rgb;
    FragColor = vec4(scene + uParams.x * bloom, 1.0);
}
