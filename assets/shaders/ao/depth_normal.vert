#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

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
out vec2 vUV;

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

    // EXACTAMENTE la expresion de pbr.vert, y no `uProjection * viewPos` que
    // seria lo natural teniendo viewPos ya calculado: esa agrupa las matrices
    // distinto -- (P * ((V*M) * p)) contra (((P*V)*M) * p) -- y en punto flotante
    // eso no es lo mismo. `invariant` garantiza que la MISMA expresion de el
    // mismo resultado; no puede salvar dos expresiones distintas.
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
