#pragma once
#include <efengine/core/Types.h>
#include <glm/glm.hpp>

namespace efengine {
namespace math {

    struct Transform {
        glm::vec3 position { 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation { 0.0f, 0.0f, 0.0f };
        glm::vec3 scale    { 1.0f, 1.0f, 1.0f };

        glm::mat4 Matrix() const;
    };
}
}