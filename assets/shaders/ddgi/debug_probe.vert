#version 450 core
// Esfera de debug de un probe, en su posicion reubicada.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 vNormal;

layout(std140, binding = 0) uniform Frame {
    mat4 uView;
    mat4 uProjection;
    mat4 uLightSpaceMatrix;
    mat4 uInvViewProjRot;
    vec4 uViewPos;
    vec4 uShadowParams;
    vec4 uIblParams;
};

layout(std140, binding = 2) uniform Object {
    mat4 uModel;
};

layout(std140, binding = 4) uniform PassParams {
    vec4 uProbeParams;   // x = probeIndex, y = modo
};

#include "ddgi/common.glsl"

void main() {
    // Escala uniforme: alcanza mat3(uModel) renormalizada.
    vNormal = normalize(mat3(uModel) * aNormal);
    // uModel trae la posicion de grilla desde la CPU; el offset vive en la GPU.
    vec3 offset = uDdgiProbeData[int(uProbeParams.x + 0.5)].xyz;
    gl_Position = uProjection * uView * (uModel * vec4(aPos, 1.0) + vec4(offset, 0.0));
}
