#include "efengine/renderer/ShadowMath.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace efengine {
namespace renderer {

    glm::mat4 ComputeDirectionalLightMatrix(
        const glm::vec3& direction, const glm::vec3& center,
        f32 orthoHalfSize, f32 distance, f32 nearPlane, f32 farPlane) {

        const glm::vec3 dir = glm::normalize(direction);
        const glm::vec3 eye = center - dir * distance;

        const glm::vec3 up = (std::abs(dir.y) > 0.999f)
                           ? glm::vec3(0.0f, 0.0f, 1.0f)
                           : glm::vec3(0.0f, 1.0f, 0.0f);

        const glm::mat4 view = glm::lookAt(eye, center, up);
        const glm::mat4 proj = glm::ortho(-orthoHalfSize, orthoHalfSize,
                                          -orthoHalfSize, orthoHalfSize,
                                          nearPlane, farPlane);
        return proj * view;
    }

}
}
