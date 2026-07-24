#version 450 core
in vec3 vDir;
out vec4 FragColor;

layout(binding = 0) uniform samplerCube uEnv;

void main() {
    // Radiancia HDR lineal directa: el tone mapping ocurre en el post (PostChain),
    // igual que la escena PBR, así el fondo queda consistente con los objetos.
    FragColor = vec4(texture(uEnv, normalize(vDir)).rgb, 1.0);
}
