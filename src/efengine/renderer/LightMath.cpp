#include "efengine/renderer/LightMath.h"

#include <algorithm>
#include <cmath>

namespace efengine {
namespace renderer {

    SpotAngleParams SpotParams(f32 cosInner, f32 cosOuter) {
        SpotAngleParams p;
        p.scale  = 1.0f / std::max(cosInner - cosOuter, 1e-4f);
        p.offset = -cosOuter * p.scale;
        return p;
    }

    f32 SpotAngular(const SpotAngleParams& p, f32 cosAngle) {
        const f32 t = std::clamp(cosAngle * p.scale + p.offset, 0.0f, 1.0f);
        return t * t;
    }

    f32 LightFalloff(f32 distance, f32 range, f32 sourceRadius) {
        if (range <= 0.0f) return 0.0f;
        const f32 d2    = distance * distance;
        const f32 ratio = d2 / (range * range);
        const f32 win   = std::clamp(1.0f - ratio * ratio, 0.0f, 1.0f);
        const f32 rMin  = std::max(sourceRadius, 0.01f);
        return (win * win) / std::max(d2, rMin * rMin);
    }

    BoundingSphere SpotBoundingSphere(const glm::vec3& position, const glm::vec3& direction,
                                      f32 range, f32 cosOuter) {
        const f32 c = std::clamp(cosOuter, 0.0f, 1.0f);
        BoundingSphere s;
        if (c < 0.70710678f) {
            s.center = position + direction * (c * range);
            s.radius = std::sqrt(1.0f - c * c) * range;
        } else {
            const f32 r = range / (2.0f * c);
            s.center = position + direction * r;
            s.radius = r;
        }
        return s;
    }

    bool SphereIntersectsAabb(const BoundingSphere& s, const glm::vec3& boxMin, const glm::vec3& boxMax) {
        const glm::vec3 d = s.center - glm::clamp(s.center, boxMin, boxMax);
        return glm::dot(d, d) <= s.radius * s.radius;
    }

    bool ConeIntersectsSphere(const glm::vec3& apex, const glm::vec3& direction,
                              f32 range, f32 cosOuter, const BoundingSphere& s) {
        const f32 c  = std::clamp(cosOuter, 0.0f, 1.0f);
        const f32 sn = std::sqrt(1.0f - c * c);
        const glm::vec3 v = s.center - apex;
        const f32 vLenSq = glm::dot(v, v);
        const f32 v1Len  = glm::dot(v, direction);
        const f32 dist   = c * std::sqrt(std::max(vLenSq - v1Len * v1Len, 0.0f)) - v1Len * sn;
        return !(dist > s.radius || v1Len > s.radius + range || v1Len < -s.radius);
    }

    bool LocalLightTouchesAabb(LightType type, const glm::vec3& position, const glm::vec3& direction,
                               f32 range, f32 cosOuter, const glm::vec3& boxMin, const glm::vec3& boxMax) {
        if (type != LightType::Spot) return SphereIntersectsAabb({ position, range }, boxMin, boxMax);

        if (!SphereIntersectsAabb(SpotBoundingSphere(position, direction, range, cosOuter), boxMin, boxMax)) {
            return false;
        }
        const BoundingSphere caja { (boxMin + boxMax) * 0.5f, glm::length(boxMax - boxMin) * 0.5f };
        return ConeIntersectsSphere(position, direction, range, cosOuter, caja);
    }

}
}
