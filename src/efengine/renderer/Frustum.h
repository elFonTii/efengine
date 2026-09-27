#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/LightMath.h>

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    // Normales hacia ADENTRO: dot(n, p) + w >= 0 es el lado visible.
    // Orden: izquierdo, derecho, abajo, arriba, cerca, lejos.
    struct Frustum {
        glm::vec4 planes[6];
    };

    // Gribb-Hartmann sobre projection * view, con planos normalizados para que
    // la distancia a una esfera salga en metros.
    Frustum ExtractFrustum(const glm::mat4& viewProjection);

    // Conservador: cerca de una esquina puede aceptar una esfera que esta afuera.
    bool SphereInFrustum(const Frustum& f, const BoundingSphere& s);

}
}
