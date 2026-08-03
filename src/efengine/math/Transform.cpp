#include "efengine/math/Transform.h"
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace efengine {
namespace math {
    
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

    Transform DecomposeTRS(const glm::mat4& m) {
        Transform t;

        // La traslacion es la cuarta columna, tal cual la puso glm::translate.
        t.position = glm::vec3(m[3]);

        // La escala es el largo de cada una de las tres primeras columnas: R es
        // ortonormal, asi que todo el estiramiento vino de S.
        glm::vec3 c0(m[0]), c1(m[1]), c2(m[2]);
        f32 sx = glm::length(c0);
        f32 sy = glm::length(c1);
        f32 sz = glm::length(c2);

        // Determinante negativo = la matriz espeja. Sin esto la rotacion sale
        // como un giro imposible; con esto el espejado queda donde corresponde,
        // en la escala.
        if (glm::determinant(glm::mat3(m)) < 0.0f) {
            sx = -sx;
            c0 = -c0;
        }

        t.scale = glm::vec3(sx, sy, sz);

        // Eje colapsado: normalizar divide por cero. La rotacion no existe, se
        // devuelve cero antes de tocar asin/atan2.
        const f32 kMinEscala = 1e-8f;
        if (std::fabs(sx) < kMinEscala || sy < kMinEscala || sz < kMinEscala) {
            t.rotation = glm::vec3(0.0f);
            return t;
        }

        // Columnas normalizadas: lo que queda es R pura.
        const glm::mat3 R(c0 / sx, c1 / sy, c2 / sz);

        // OJO: glm indexa [columna][fila]. R[2][1] de aca es el R[1][2] de la
        // deduccion en el plan/spec.
        const f32 senoPitch = -R[2][1];
        const f32 pitch     = std::asin(glm::clamp(senoPitch, -1.0f, 1.0f));

        f32 yaw, roll;
        if (std::fabs(senoPitch) < 0.9999f) {
            yaw  = std::atan2(R[2][0], R[2][2]);
            roll = std::atan2(R[0][1], R[1][1]);
        } else {
            // Gimbal lock: cos(pitch) ~ 0 y yaw y roll dejan de ser separables.
            // Se fija roll en cero y todo el giro restante va al yaw.
            yaw  = std::atan2(-R[0][2], R[0][0]);
            roll = 0.0f;
        }

        t.rotation = glm::degrees(glm::vec3(pitch, yaw, roll));
        return t;
    }
}
}