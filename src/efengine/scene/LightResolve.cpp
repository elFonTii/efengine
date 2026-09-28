#include "efengine/scene/LightResolve.h"

#include <efengine/math/ColorTemperature.h>

#include <algorithm>
#include <cmath>

namespace efengine {
namespace scene {

    namespace {
        constexpr f32 kMinRange   = 0.01f;
        constexpr f32 kMaxConeDeg = 89.0f;
    }

    glm::vec3 EffectiveLightColor(const LightAttachment& a) {
        glm::vec3 c = glm::max(a.color, glm::vec3(0.0f));
        if (a.useTemperature) c *= math::KelvinToLinearRgb(a.temperatureK);
        return c * std::max(a.intensity, 0.0f);
    }

    renderer::Light ResolveLight(const LightAttachment& a, const glm::mat4& world) {
        renderer::Light l;
        switch (a.kind) {
            case LightKind::Point:       l.type = renderer::LightType::Point;       break;
            case LightKind::Spot:        l.type = renderer::LightType::Spot;        break;
            case LightKind::Directional: l.type = renderer::LightType::Directional; break;
        }

        l.position = glm::vec3(world[3]);
        // w = 0 anula la traslacion; normalizar descarta la escala del nodo.
        const glm::vec3 fwd = glm::vec3(world * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
        const f32 largo = glm::length(fwd);
        l.direction = (largo > 0.0f) ? fwd / largo : glm::vec3(0.0f, 0.0f, -1.0f);

        l.color        = EffectiveLightColor(a);
        l.range        = std::max(a.range, kMinRange);
        l.sourceRadius = std::clamp(a.sourceRadius, 0.0f, l.range * 0.99f);

        const f32 outer = std::clamp(a.outerConeDeg, 0.0f, kMaxConeDeg);
        const f32 inner = std::clamp(a.innerConeDeg, 0.0f, outer);
        l.cosInner = std::cos(glm::radians(inner));
        l.cosOuter = std::cos(glm::radians(outer));

        l.castShadows = a.castShadows;
        return l;
    }

}
}
