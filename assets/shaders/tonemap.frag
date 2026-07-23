#version 450 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScreenTexture;   // color HDR lineal de la escena (RGBA16F)
uniform float     uExposure;        // exposición lineal (1.0 = neutra)

// ACES filmic tone mapping — aproximación de Narkowicz.
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
// Comprime el rango HDR a [0,1] con un "shoulder" que doma los highlights sin
// quemarlos de golpe (a diferencia del Reinhard puro que teníamos antes).
vec3 ACESFilmic(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(uScreenTexture, vUV).rgb;

    hdr *= uExposure;               // exposición (multiplicador lineal)
    vec3 ldr = ACESFilmic(hdr);     // HDR → LDR (tone mapping)
    ldr = pow(ldr, vec3(1.0 / 2.2)); // gamma: lineal → sRGB (una sola vez, acá)

    FragColor = vec4(ldr, 1.0);
}
