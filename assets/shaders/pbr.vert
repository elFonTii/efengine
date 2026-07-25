#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aTangent;

/*
Cada vertice de un modelo tiene un vector Normal (hacia dónde mira )
una vector Tangente (hacia dónde va la textura en X) y un Bitangente (hacia dónde va la textura en Y). 
A esto se le llama la Matriz TBN.
*/

out vec3 vFragPos;
out vec2 vUV;
out mat3 vTBN;

// Datos per-frame en un solo UBO std140 (espejo de renderer/FrameData.h).
// El bloque debe declararse idéntico en todos los stages que lo usan.
#define MAX_LIGHTS 4
layout(std140, binding = 0) uniform FrameData {
    mat4 uView;
    mat4 uProjection;
    mat4 uViewProj;
    mat4 uLightSpaceMatrix;
    vec4 uCamPos;          // xyz = posición de cámara
    vec4 uSunDirection;    // xyz = dirección del sol, w = sombra on (0/1)
    vec4 uSunColor;        // xyz = color*intensidad, w = ambientFactor
    vec4 uShadowParams;    // x = biasMin, y = biasMax
    vec4 uPointPositions[MAX_LIGHTS];
    vec4 uPointColors[MAX_LIGHTS];
    ivec4 uCounts;         // x = cantidad de point lights
};

uniform mat4 uModel;

void main() {
    vFragPos = vec3(uModel * vec4(aPos, 1.0)); // de vec4 a vec3
    vUV = aUV;

    
    mat3 normalMatrix = transpose(inverse(mat3(uModel))); 
    
    // Mover tangente y normal del espacio modelo al espacio mundo (esto deforma los vectores)
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);

    T = normalize (T - dot(T, N) * N); // dot -> Producto Punto (sirve para determinar si 2 vectores son perpendiculares)

    // T/N ahora son perpendiculares, toca perpendicularizar el Bitangente
    vec3 B = cross(N, T); // cross -> Producto Cruz (Toma dos vectores y devuelve un nuevo vector que porsupuesto es perpendicular al plano que contiene a T y N)

    vTBN = mat3(T, B, N); // se construye la matriz TBN con los nuevos vectores perpendiculares/ortogonales (es lo mismo)

    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}