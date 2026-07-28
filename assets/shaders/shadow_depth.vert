#version 450 core
layout (location = 0) in vec3 aPos;

// El pase de sombra corre ANTES de BeginScene, asi que no puede leer el bloque
// Frame: su matriz light-space viaja por PassParams.
layout(std140, binding = 4) uniform PassParams {
    mat4 uLightSpaceMatrix;
};

layout(std140, binding = 2) uniform Object {
    mat4 uModel;
};

void main() {
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPos, 1.0);
}
