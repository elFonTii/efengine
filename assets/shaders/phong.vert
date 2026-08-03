#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
// location 3 (aTangent) existe en el VAO pero Phong no la usa.

out vec3 vFragPos;   // posición en espacio mundo
out vec3 vNormal;    // normal en espacio mundo
out vec2 vUV;

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
    vFragPos = vec3(uModel * vec4(aPos, 1.0));
    // normal matrix: corrige la normal ante escalados no uniformes.
    vNormal  = mat3(transpose(inverse(uModel))) * aNormal;
    vUV      = aUV;

    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}