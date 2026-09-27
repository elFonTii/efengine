#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Light.h>

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    // Espejos C++ de common/lights.glsl y lights/cluster_cull.comp. El GLSL no
    // se testea headless; esto si, y el shader tiene que dar lo mismo.

    struct BoundingSphere {
        glm::vec3 center { 0.0f };
        f32       radius = 0.0f;
    };

    // Atenuacion angular de Frostbite: t = saturate(cosAngle * scale + offset), t^2.
    struct SpotAngleParams {
        f32 scale  = 0.0f;
        f32 offset = 1.0f;
    };
    SpotAngleParams SpotParams(f32 cosInner, f32 cosOuter);
    f32             SpotAngular(const SpotAngleParams& p, f32 cosAngle);

    // Ventana de Karis sobre inverse-square: 0 exacto en distance == range.
    f32 LightFalloff(f32 distance, f32 range, f32 sourceRadius);

    // Esfera que contiene el sector esferico del spot (Wronski 2017).
    BoundingSphere SpotBoundingSphere(const glm::vec3& position, const glm::vec3& direction,
                                      f32 range, f32 cosOuter);

    bool SphereIntersectsAabb(const BoundingSphere& s, const glm::vec3& boxMin, const glm::vec3& boxMax);

    // "Cull that cone" (Wronski 2017). direction normalizada.
    bool ConeIntersectsSphere(const glm::vec3& apex, const glm::vec3& direction,
                              f32 range, f32 cosOuter, const BoundingSphere& s);

    // La decision del compute por cada (luz, cluster). Conservadora: puede
    // aceptar de mas, nunca rechaza una caja que la luz ilumina.
    bool LocalLightTouchesAabb(LightType type, const glm::vec3& position, const glm::vec3& direction,
                               f32 range, f32 cosOuter, const glm::vec3& boxMin, const glm::vec3& boxMax);

}
}
