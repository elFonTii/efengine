#pragma once
#include <glm/glm.hpp>

namespace efengine {
namespace renderer {
    struct DirectionalLight {
        glm::vec3 direction;
        glm::vec3 color;
    };
}
}
