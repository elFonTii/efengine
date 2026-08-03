#version 450 core
// Cielo del target de captura de probes. Reusa skybox.vert verbatim: lo unico
// que ese vertex shader necesita es uInvViewProjRot, y el FrameBlock que sube
// DdgiPass por cara ya lo trae.
//
// La diferencia con skybox.frag es el alfa: la captura guarda distancia ahi, y
// el cielo esta "infinitamente lejos". Con 1e4 el test de Chebyshev nunca lo
// cuenta como oclusion, que es lo correcto -- el cielo ilumina, no tapa.

in vec3 vDir;

out vec4 FragColor;

layout(binding = 0) uniform samplerCube uEnvMap;

void main() {
    vec3 sky = texture(uEnvMap, normalize(vDir)).rgb;
    FragColor = vec4(sky, 1e4);
}
