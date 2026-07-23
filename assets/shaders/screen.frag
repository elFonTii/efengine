#version 450 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScreenTexture;   // color attachment del framebuffer de escena

void main() {
    FragColor = texture(uScreenTexture, vUV);   // passthrough: copia sin tocar
}
