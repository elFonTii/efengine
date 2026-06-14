#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/VertexLayout.h>

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 tangent;

    };

    static_assert(sizeof(Vertex) == 11 * sizeof(f32),
                  "Vertex debe quedar empaquetado sin padding");

    VertexLayout StandardVertexLayout();
}
}