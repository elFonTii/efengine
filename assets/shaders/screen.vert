#version 330 core
layout (location = 0) in vec2 aPos;   // posición en NDC [-1, 1]
layout (location = 1) in vec2 aUV;    // coordenada de textura [0, 1]

out vec2 vUV;

void main() {
    vUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);   // z = 0 → pasa el depth test con depth limpio (1.0)
}
