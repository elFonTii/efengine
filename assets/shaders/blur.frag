#version 450 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uImage;      // imagen a desenfocar
uniform bool uHorizontal;      // true = pasada horizontal; false = vertical

// Gaussiano separable de 5 taps (pesos normalizados, suman ~1.0).
const float wgt[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texel = 1.0 / vec2(textureSize(uImage, 0));   // tamano de un texel
    vec2 dir = uHorizontal ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);

    vec3 c = texture(uImage, vUV).rgb * wgt[0];
    for (int i = 1; i < 5; ++i) {
        c += texture(uImage, vUV + dir * float(i)).rgb * wgt[i];
        c += texture(uImage, vUV - dir * float(i)).rgb * wgt[i];
    }
    FragColor = vec4(c, 1.0);
}
