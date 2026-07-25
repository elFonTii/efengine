#pragma once

#include <efengine/core/Types.h>
#include <glm/glm.hpp>

namespace efengine {
namespace math {

    // Caja alineada a ejes en espacio local o mundial. Agregado trivial (RO0).
    // Un Aabb con min > max se considera vacío (estado por defecto).
    struct Aabb {
        glm::vec3 min {  1.0f };
        glm::vec3 max { -1.0f };
    };

    // Funciones libres puras: sin estado ni llamadas GL, testeables con doctest.
    bool      AabbIsEmpty(const Aabb& box);
    Aabb      AabbFromPoints(const glm::vec3* points, usize count);
    Aabb      MergeAabb(const Aabb& a, const Aabb& b);
    // Transforma las 8 esquinas y devuelve la caja que las encierra.
    Aabb      TransformAabb(const glm::mat4& m, const Aabb& box);
    glm::vec3 AabbCenter(const Aabb& box);
    glm::vec3 AabbExtents(const Aabb& box); // semiejes (mitad del tamaño)

}
}
