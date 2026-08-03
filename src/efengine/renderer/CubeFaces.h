#pragma once
#include <efengine/core/Types.h>
#include <efengine/core/Assert.h>

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    inline constexpr u32 kCubeFaceCount = 6u;

    // Base de camara de una cara de cubemap: lo que come glm::lookAt.
    struct CubeFaceBasis {
        glm::vec3 forward;
        glm::vec3 up;
    };

    // Los 6 pares (forward, up) de la convencion estandar de cubemaps de GL, que
    // es la que usa assets/shaders/common/cubeface.glsl.
    //
    // Lo que los hace correctos: para lookAt(c, c + forward, up) con fov 90 y
    // aspect 1, la direccion del NDC (a, b) es normalize(forward + a*r + b*u),
    // con r = normalize(cross(forward, up)) y u = cross(r, forward). Eso tiene
    // que dar lo mismo que dirForFace(cara, vec2(a, b)). El test de esta tarea
    // verifica exactamente esa igualdad.
    inline const CubeFaceBasis& CubeFace(u32 face) {
        static const CubeFaceBasis kFaces[kCubeFaceCount] = {
            { {  1.0f,  0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } },  // 0 +X
            { { -1.0f,  0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } },  // 1 -X
            { {  0.0f,  1.0f,  0.0f }, { 0.0f,  0.0f,  1.0f } },  // 2 +Y
            { {  0.0f, -1.0f,  0.0f }, { 0.0f,  0.0f, -1.0f } },  // 3 -Y
            { {  0.0f,  0.0f,  1.0f }, { 0.0f, -1.0f,  0.0f } },  // 4 +Z
            { {  0.0f,  0.0f, -1.0f }, { 0.0f, -1.0f,  0.0f } },  // 5 -Z
        };
        EF_ASSERT(face < kCubeFaceCount, "CubeFace: indice de cara fuera de rango");
        return kFaces[face];
    }

    // Mirror en C++ de dirForFace de assets/shaders/common/cubeface.glsl.
    // Existe SOLO para que el test pueda atar los pares de arriba a la tabla.
    // Ningun codigo de runtime deberia necesitarlo: si lo estas llamando fuera
    // de un test, probablemente querias CubeFace().
    inline glm::vec3 DirForFace(u32 face, glm::vec2 uv) {
        glm::vec3 d(0.0f);
        switch (face) {
            case 0u: d = glm::vec3( 1.0f,  -uv.y, -uv.x); break;  // +X
            case 1u: d = glm::vec3(-1.0f,  -uv.y,  uv.x); break;  // -X
            case 2u: d = glm::vec3( uv.x,  1.0f,   uv.y); break;  // +Y
            case 3u: d = glm::vec3( uv.x, -1.0f,  -uv.y); break;  // -Y
            case 4u: d = glm::vec3( uv.x,  -uv.y,  1.0f); break;  // +Z
            default: d = glm::vec3(-uv.x,  -uv.y, -1.0f); break;  // -Z
        }
        return glm::normalize(d);
    }

}
}
