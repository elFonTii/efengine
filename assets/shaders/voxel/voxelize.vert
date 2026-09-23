#version 450 core
// Voxelizacion: proyecta la escena ortograficamente sobre UN eje y deja que el
// fragment shader estampe el voxel. El pase corre tres veces, una por eje, y
// uViewProj dice cual.
//
// Tres pasadas en vez de un geometry shader que elige el eje dominante por
// triangulo: corre una sola vez por escena, asi que la simplicidad vale mas que
// el costo. Ademas tapa los huecos que deja rasterizar geometria delgada desde
// un solo angulo, que es lo que la rasterizacion conservativa resolveria con una
// extension que no es core.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

layout(std140, binding = 2) uniform Object {
    mat4 uModel;
};

// El motor no tiene uniforms sueltos: todo dato de shader viaja por UBO.
layout(std140, binding = 4) uniform PassParams {
    mat4 uViewProj;     // ortografica del eje de esta pasada
    vec4 uGridOrigin;   // .xyz = esquina minima del grid
    vec4 uGridParams;   // x = voxelSize (m), y = resolucion por eje
};

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    vec4 mundo = uModel * vec4(aPos, 1.0);
    vWorldPos  = mundo.xyz;
    // Sin matriz normal: la escala no uniforme la corrige el normalize del
    // fragment, y el voxel guarda la normal con 8 bits por canal. Invertir y
    // transponer por vertice para eso no paga.
    vNormal = mat3(uModel) * aNormal;
    vUV     = aUV;

    gl_Position = uViewProj * mundo;
}
