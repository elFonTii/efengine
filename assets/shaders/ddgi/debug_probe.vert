#version 450 core
// Esfera de debug de un probe. La normal en espacio mundo es lo unico que el
// fragment necesita: con ella indexa el tile octaedrico del probe.

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

void main() {
    // La esfera se escala uniforme, asi que la normal no necesita la inversa
    // transpuesta: alcanza mat3(uModel) renormalizada.
    vNormal = normalize(mat3(uModel) * aNormal);
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
