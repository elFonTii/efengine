#version 450 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;      // escena HDR full-res
uniform sampler2D uBloom;      // bloom desenfocado (1/2 res; LINEAR upscalea)
uniform float uIntensity;      // fuerza del halo

// Composicion aditiva en HDR lineal, antes del tonemap.
void main() {
    vec3 scene = texture(uScene, vUV).rgb;
    vec3 bloom = texture(uBloom, vUV).rgb;
    FragColor = vec4(scene + uIntensity * bloom, 1.0);
}
