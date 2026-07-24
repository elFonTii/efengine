#pragma once
#include <glm/glm.hpp>

namespace efengine {
namespace renderer {
    // Luz direccional (sol): sin posición; 'direction' es hacia dónde viaja la luz.
    // 'color' es color * intensidad (mismo criterio HDR que PointLight).
    struct DirectionalLight {
        glm::vec3 direction;
        glm::vec3 color;
    };
}
}
