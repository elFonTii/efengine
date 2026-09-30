#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

#include "common/object.glsl"

// PassParams propio (binding 4): este pase corre ANTES de BeginScene.
layout(std140, binding = 4) uniform PrepassParams {
    mat4 uView;
    mat4 uProjection;
    mat4 uViewProjNoJitter;
    mat4 uPrevViewProjNoJitter;
};

out vec3 vViewNormal;
out vec3 vViewPos;
out vec2 vUV;
out vec4 vClipCur;
out vec4 vClipPrev;

// Ver el comentario largo de pbr.vert: el forward dibuja con GL_EQUAL contra la
// profundidad que deja este pase, y para eso los dos programas tienen que
// producir gl_Position con los MISMOS BITS.
invariant gl_Position;

void main() {
    mat4 modelView = uView * uModel;
    vec4 viewPos   = modelView * vec4(aPos, 1.0);

    vViewPos    = viewPos.xyz;
    // La inversa transpuesta para que la escala no uniforme no tuerza la normal.
    vViewNormal = mat3(transpose(inverse(modelView))) * aNormal;
    vUV         = aUV;

    // Sin jitter las dos: la velocidad no tiene que temblar con el Halton.
    vClipCur  = uViewProjNoJitter     * uModel     * vec4(aPos, 1.0);
    vClipPrev = uPrevViewProjNoJitter * uPrevModel * vec4(aPos, 1.0);

    // EXACTAMENTE la expresion de pbr.vert, y no `uProjection * viewPos` que
    // seria lo natural teniendo viewPos ya calculado: esa agrupa las matrices
    // distinto -- (P * ((V*M) * p)) contra (((P*V)*M) * p) -- y en punto flotante
    // eso no es lo mismo. `invariant` garantiza que la MISMA expresion de el
    // mismo resultado; no puede salvar dos expresiones distintas.
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
