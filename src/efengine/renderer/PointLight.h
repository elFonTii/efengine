#pragma once

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {
    struct PointLight {
        glm::vec3 position;
        glm::vec3 color;
    };
}
}