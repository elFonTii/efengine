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

    // Inversa de Transform::Matrix(): saca position, rotation y scale de una
    // matriz afin. Los angulos salen en GRADOS y en el orden YXZ que compone
    // Matrix(), no el orden default de glm.
    //
    // Exacta solo si la matriz es TRS pura. Si algun ancestro mezcla escala no
    // uniforme con rotacion, la matriz tiene cizalladura y NO existe ningun TRS
    // que la represente: se devuelve la mejor aproximacion (traslacion y
    // rotacion exactas, escala por largo de columna, shear descartado). Es la
    // misma limitacion que tienen Unity y Unreal.
    //
    // Si algun eje tiene escala ~0 la rotacion es indeterminada y sale en cero,
    // pero nunca NaN.
    Transform DecomposeTRS(const glm::mat4& m);
}
}