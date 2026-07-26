#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Vertex.h>

#include <glm/glm.hpp>

#include <vector>

namespace efengine {
namespace renderer {

    // Axis-Aligned-Bounding-Box - guarda la esquina con x,y,z mínimas y la esquina con x,y,z máximas.
    // sirve para responder rapido y aproximadamente donde está un objeto
    struct AABB {
        glm::vec3 min { 0.0f };
        glm::vec3 max { 0.0f };

        bool Valid() const;

        glm::vec3 Center() const;

        glm::vec3 Extents() const;

        f32 Radius() const;

        // método de Arvo: el centro
        // va como punto por 'm' y los extents se escalan con el abs() del
        // bloque 3x3. Rotar agranda la caja; trasladar no.
        // Llamarlo sobre una AABB inválida es error del que llama: los
        // infinitos de Empty() producen NaN. Chequear Valid() primero.
        AABB Transformed(const glm::mat4& m) const;

        static AABB Merge(const AABB& a, const AABB& b);

        static AABB Empty();
    };

    AABB ComputeBounds(const std::vector<Vertex>& vertices);

}
}
