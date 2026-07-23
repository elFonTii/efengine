#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
// location 3 (aTangent) existe en el VAO pero Phong no la usa.

out vec3 vFragPos;   // posición en espacio mundo
out vec3 vNormal;    // normal en espacio mundo
out vec2 vUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    vFragPos = vec3(uModel * vec4(aPos, 1.0));
    // normal matrix: corrige la normal ante escalados no uniformes.
    vNormal  = mat3(transpose(inverse(uModel))) * aNormal;
    vUV      = aUV;

    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}