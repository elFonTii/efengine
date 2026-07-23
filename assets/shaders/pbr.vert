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

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

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