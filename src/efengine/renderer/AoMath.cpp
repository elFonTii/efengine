#include "efengine/renderer/AoMath.h"

#include <algorithm>

namespace efengine {
namespace renderer {

    glm::vec2 AoProjInfo(const glm::mat4& projection) {
        // Una proyeccion degenerada (matriz identidad en un test) daria 1/0.
        const f32 px = (projection[0][0] != 0.0f) ? 1.0f / projection[0][0] : 0.0f;
        const f32 py = (projection[1][1] != 0.0f) ? 1.0f / projection[1][1] : 0.0f;
        return glm::vec2(px, py);
    }

    f32 AoProjScale(const glm::mat4& projection, u32 height) {
        // 0.5 * alto / tan(fovY/2), y P[1][1] ES 1/tan(fovY/2).
        return 0.5f * static_cast<f32>(height) * projection[1][1];
    }

    AoSettings SanitizeAoSettings(const AoSettings& settings) {
        AoSettings s = settings;
        s.radius          = std::clamp(s.radius,          0.0f,  100.0f);
        s.intensity       = std::clamp(s.intensity,       0.0f,  8.0f);
        s.thickness       = std::clamp(s.thickness,       0.01f, 1.0f);
        s.slices          = std::clamp(s.slices,          1,     8);
        s.steps           = std::clamp(s.steps,           1,     32);
        s.maxScreenRadius = std::clamp(s.maxScreenRadius, 1.0f,  512.0f);
        return s;
    }

    AoPassBlock MakeAoPassBlock(const glm::mat4& view, const glm::mat4& projection,
                                const AoSettings& settings, u32 width, u32 height,
                                i32 blurDirection) {
        // Se sanea aca y no en el caller: este es el ultimo punto antes de que
        // los valores lleguen al shader, donde un cero se vuelve NaN silencioso.
        const AoSettings s = SanitizeAoSettings(settings);

        const glm::vec2 info = AoProjInfo(projection);
        const f32 invW = (width  > 0u) ? 1.0f / static_cast<f32>(width)  : 0.0f;
        const f32 invH = (height > 0u) ? 1.0f / static_cast<f32>(height) : 0.0f;

        AoPassBlock b {};
        b.viewToWorld = glm::inverse(view);
        b.projInfo    = glm::vec4(info.x, info.y, invW, invH);
        b.params0     = glm::vec4(s.radius, s.thickness, s.intensity, s.maxScreenRadius);
        b.params1     = glm::vec4(AoProjScale(projection, height), 0.0f, 0.0f, 0.0f);
        b.counts      = glm::ivec4(s.slices, s.steps, blurDirection,
                                   static_cast<i32>(s.debugView));
        return b;
    }

    AoBlock MakeAoBlock(const AoContext& ctx) {
        const bool on = (ctx.enabled && ctx.texture != null);

        AoBlock b {};
        b.params = glm::vec4(on ? 1.0f : 0.0f,
                             (on && ctx.bentNormal)  ? 1.0f : 0.0f,
                             (on && ctx.multiBounce) ? 1.0f : 0.0f,
                             static_cast<f32>(ctx.debugView));
        return b;
    }

}
}
