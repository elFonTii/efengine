#version 450 core
layout (location = 0) in vec3 aPos;

// El pase de sombra corre ANTES de BeginScene, asi que no puede leer el bloque
// Frame: su matriz light-space viaja por PassParams.
layout(std140, binding = 4) uniform PassParams {
    mat4 uLightSpaceMatrix;
};

#include "common/object.glsl"

void main() {
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPos, 1.0);
}
