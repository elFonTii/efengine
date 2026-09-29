#version 450 core
// Skybox sobre el quad fullscreen: reconstruye la dirección de vista de mundo
// por vértice a partir de la esquina en NDC y la inversa de (proj * rotación de
// vista). Sin traslación → el entorno se ve "infinitamente lejos".
layout(location = 0) in vec2 aPos;   // esquina del quad, en NDC [-1,1]

out vec3 vDir;                        // dirección de vista en espacio mundo

// uInvViewProjRot lo calcula MakeFrameBlock: el skybox no tiene uniforms propios.
#include "common/frame.glsl"

void main() {
    vec4 world = uInvViewProjRot * vec4(aPos, 1.0, 1.0);  // punto en el plano lejano
    vDir = world.xyz / world.w;
    gl_Position = vec4(aPos, 1.0, 1.0);                    // z=w → profundidad lejana
}
