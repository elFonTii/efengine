#pragma once
#include <efengine/core/Types.h>

#include <glm/glm.hpp>

#include <vector>

namespace efengine {

namespace renderer { class Model; }

namespace gameplay {

    // Geometria de colision en el formato que pide physics::ShapeDesc: las
    // posiciones aplanadas a xyz consecutivos.
    struct CollisionMesh {
        std::vector<f32> positions;
        std::vector<u32> indices;
    };

    // Suma una submalla a 'out'. Los indices de cada submalla son locales a la
    // suya, asi que se corren por los vertices ya acumulados. Una submalla sin
    // posiciones o sin indices no aporta nada y se ignora.
    void AppendSubmesh(CollisionMesh& out,
                       const std::vector<glm::vec3>& positions,
                       const std::vector<u32>& indices);

    // Camina las submallas del modelo y las concatena. No usa GL, pero construir
    // un Model si, asi que esta mitad no tiene test propio: lo que se testea es
    // AppendSubmesh.
    CollisionMesh BuildCollisionMesh(const renderer::Model& model);

}
}
