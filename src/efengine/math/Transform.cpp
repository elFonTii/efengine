#include "efengine/math/Transform.h"
#include <glm/gtc/matrix_transform.hpp>

namespace efengine {
namespace math {
    
    // Matrix tiene que devolver T * R * S
    // T: Translación | R: Rotación | S: Tamaño
    glm::mat4 Transform::Matrix() const {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

        glm::mat4 R = glm::mat4(1.0f);
        R = glm::rotate(R, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // YAW (Y)
        R = glm::rotate(R, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // PITCH (X)
        R = glm::rotate(R, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // ROLL (Z)

        return T * R * S;
    }
}
}