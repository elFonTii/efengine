#include "efengine/renderer/LightPacking.h"

#include <algorithm>
#include <cmath>

namespace efengine {
namespace renderer {

    GpuLight PackGpuLight(const Light& l) {
        GpuLight g {};
        g.positionRange = glm::vec4(l.position, l.range);
        g.colorRadius   = glm::vec4(l.color, l.sourceRadius);
        g.directionType = glm::vec4(l.direction, (l.type == LightType::Spot) ? 1.0f : 0.0f);

        const SpotAngleParams sp = SpotParams(l.cosInner, l.cosOuter);
        const f32 c = std::clamp(l.cosOuter, 0.0f, 1.0f);
        g.spotParams = glm::vec4(sp.scale, sp.offset, c, std::sqrt(1.0f - c * c));

        g.reserved = glm::vec4(-1.0f, l.castShadows ? 1.0f : 0.0f, 0.0f, 0.0f);
        return g;
    }

    BoundingSphere LocalLightBounds(const Light& l) {
        if (l.type == LightType::Spot) return SpotBoundingSphere(l.position, l.direction, l.range, l.cosOuter);
        return BoundingSphere{ l.position, l.range };
    }

    void PackLights(const std::vector<Light>& lights, const Frustum& frustum, PackedLights& out) {
        out.block = LightsBlock{};
        out.locals.clear();
        out.visible.clear();
        out.totalLocals      = 0u;
        out.totalDirectional = 0u;

        u32 dirs = 0u;
        auto agregarDireccional = [&](const Light& l) {
            if (dirs >= kMaxDirectionalLights) return;
            out.block.dirDirection[dirs] = glm::vec4(l.direction, 0.0f);
            out.block.dirColor[dirs]     = glm::vec4(l.color, l.primarySun ? 1.0f : 0.0f);
            ++dirs;
        };

        // El sol primero: el recorte a 4 no se puede llevar puesto justo al que tiene sombra.
        for (const Light& l : lights) {
            if (l.type == LightType::Directional && l.primarySun) agregarDireccional(l);
        }

        for (const Light& l : lights) {
            if (l.type == LightType::Directional) {
                ++out.totalDirectional;
                if (!l.primarySun) agregarDireccional(l);
                continue;
            }
            ++out.totalLocals;
            if (out.locals.size() >= kMaxLocalLights) continue;

            const u32 indice = static_cast<u32>(out.locals.size());
            out.locals.push_back(PackGpuLight(l));
            if (SphereInFrustum(frustum, LocalLightBounds(l))) out.visible.push_back(indice);
        }

        out.block.counts = glm::uvec4(static_cast<u32>(out.locals.size()), dirs,
                                      static_cast<u32>(out.visible.size()), 0u);
    }

}
}
