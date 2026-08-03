#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// El mismo bloque Object (binding 2) que sube Renderer::Submit por item.
layout(std140, binding = 2) uniform ObjectParams {
    mat4 uModel;
};

// PassParams propio (binding 4): este pase corre ANTES de BeginScene, asi que
// el bloque Frame de la camara todavia no existe. Misma restriccion y misma
// solucion que shadow_depth.vert.
layout(std140, binding = 4) uniform AoPrepassParams {
    mat4 uView;
    mat4 uProjection;
};

out vec3 vViewNormal;
out vec3 vViewPos;

void main() {
    mat4 modelView = uView * uModel;
    vec4 viewPos   = modelView * vec4(aPos, 1.0);

    vViewPos    = viewPos.xyz;
    // La inversa transpuesta para que la escala no uniforme no tuerza la normal.
    vViewNormal = mat3(transpose(inverse(modelView))) * aNormal;

    gl_Position = uProjection * viewPos;
}
