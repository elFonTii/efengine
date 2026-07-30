#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Vertex.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace efengine {
namespace renderer {

    inline constexpr u32 kBoxFaceCount = 6u;

    // Indices de cara. El orden es parte del contrato: holeFace lo usa, y el
    // payload del .efe lo guarda como numero.
    enum class BoxFaceIndex : u32 {
        XNeg = 0u, XPos = 1u, YNeg = 2u, YPos = 3u, ZNeg = 4u, ZPos = 5u
    };

    struct BoxParams {
        glm::vec3 half     { 4.0f, 2.0f, 4.0f };  // semi-extents en METROS
        u32       inward   = 1u;    // 1 = normales hacia adentro (sala); 0 = hacia afuera (bloque)
        u32       holeFace = 6u;    // cara con abertura, 0..5; 6 o mas = ninguna
        // Rect de la abertura en coords locales de la cara, CENTRADAS en ella:
        // (uMin, vMin, uMax, vMax). Se clampea a la cara y se ignora si queda vacio.
        glm::vec4 hole     { 0.0f };
        f32       uvPerMeter = 1.0f;

        template <class Ar>
        void Serialize(Ar& ar) {
            ar.Field(half);
            ar.Field(inward);
            ar.Field(holeFace);
            ar.Field(hole);
            ar.Field(uvPerMeter);
        }
    };

    // Una entrada por cara, siempre kBoxFaceCount y siempre en el orden de
    // BoxFaceIndex. El corte por cara no es cosmetico: es lo que permite que cada
    // pared tenga su propio material, porque MaterialMap indexa por nombre de
    // submesh.
    struct BoxFace {
        std::string         name;
        std::vector<Vertex> vertices;
        std::vector<u32>    indices;
    };

    // Pura, sin GL: es la que se testea.
    std::vector<BoxFace> BoxMeshData(const BoxParams& p);

    // Sube cada cara como un Mesh. Necesita contexto GL.
    Model MakeBoxModel(const BoxParams& p);

}
}
