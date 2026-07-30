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

    // Angulos de Euler en GRADOS (pitch, yaw, roll) que hacen que Transform::Matrix()
    // oriente el -Z local hacia 'forward'. Roll queda en cero: una direccion sola
    // no lo determina.
    //
    // Es la inversa EXACTA de lo que hace SceneGraph para sacar la direccion del
    // sol: normalize(worldMatrix * vec4(0,0,-1,0)). Si se cambia el orden de
    // composicion de Matrix(), hay que cambiar esto tambien.
    glm::vec3 EulerFromForward(const glm::vec3& forward);
}
}