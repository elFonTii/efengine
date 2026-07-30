#include "efengine/math/Transform.h"
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

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

    glm::vec3 EulerFromForward(const glm::vec3& forward) {
        const f32 largo = glm::length(forward);
        if (largo < 1e-6f) return glm::vec3(0.0f);

        const glm::vec3 f = forward / largo;

        // Matrix() compone Ry(yaw) * Rx(pitch) * Rz(roll), asi que
        //   R * (0,0,-1) = (-cos(pitch)*sin(yaw), sin(pitch), -cos(pitch)*cos(yaw))
        // Despejando: pitch sale del componente Y, y yaw de atan2 sobre X y Z.
        // atan2 y no un cociente: cuando el sol mira derecho para abajo
        // cos(pitch) vale 0 y cualquier division explota.
        const f32 pitch = glm::degrees(std::asin(glm::clamp(f.y, -1.0f, 1.0f)));
        const f32 yaw   = glm::degrees(std::atan2(-f.x, -f.z));

        return glm::vec3(pitch, yaw, 0.0f);
    }
}
}