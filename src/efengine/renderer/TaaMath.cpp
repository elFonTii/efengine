#include "efengine/renderer/TaaMath.h"

#include <efengine/renderer/FrameView.h>

#include <algorithm>
#include <cmath>

namespace efengine {
namespace renderer {

    glm::vec3 RgbToYCoCg(const glm::vec3& c) {
        return glm::vec3( 0.25f * c.r + 0.5f * c.g + 0.25f * c.b,
                          0.5f  * c.r              - 0.5f  * c.b,
                         -0.25f * c.r + 0.5f * c.g - 0.25f * c.b);
    }

    glm::vec3 YCoCgToRgb(const glm::vec3& c) {
        const f32 t = c.x - c.z;
        return glm::vec3(t + c.y, c.x + c.z, t - c.y);
    }

    glm::vec3 ClipToAabb(const glm::vec3& h, const glm::vec3& mn, const glm::vec3& mx) {
        const glm::vec3 centro = 0.5f * (mx + mn);
        const glm::vec3 medio  = 0.5f * (mx - mn) + glm::vec3(1e-5f);
        const glm::vec3 v      = h - centro;
        const glm::vec3 u      = glm::abs(v / medio);
        const f32 a = std::max(u.x, std::max(u.y, u.z));
        return a > 1.0f ? centro + v / a : h;
    }

    glm::vec4 CatmullRomWeights(f32 t) {
        return glm::vec4(t * (-0.5f + t * (1.0f - 0.5f * t)),
                         1.0f + t * t * (-2.5f + 1.5f * t),
                         t * (0.5f + t * (2.0f - 1.5f * t)),
                         t * t * (-0.5f + 0.5f * t));
    }

    glm::vec2 ClipToUv(const glm::vec4& clip) {
        return glm::vec2(clip) / clip.w * 0.5f + 0.5f;
    }

    glm::vec2 VelocityUv(const glm::vec4& clipCur, const glm::vec4& clipPrev) {
        return ClipToUv(clipCur) - ClipToUv(clipPrev);
    }

    glm::vec2 CameraVelocityUv(const glm::vec2& uv, f32 depth,
                               const glm::mat4& invViewProjNoJitter,
                               const glm::mat4& prevViewProjNoJitter,
                               const glm::vec2& jitterUv) {
        const glm::vec2 uvSinJitter = uv - jitterUv;
        const glm::vec4 ndc(uvSinJitter * 2.0f - 1.0f, depth * 2.0f - 1.0f, 1.0f);
        glm::vec4 mundo = invViewProjNoJitter * ndc;
        mundo /= mundo.w;
        return uvSinJitter - ClipToUv(prevViewProjNoJitter * mundo);
    }

    f32 Luma(const glm::vec3& c) {
        return glm::dot(c, glm::vec3(0.2126f, 0.7152f, 0.0722f));
    }

    glm::vec3 TaaBlend(const glm::vec3& cur, const glm::vec3& hist, f32 alpha) {
        if (alpha >= 1.0f) return cur;
        const f32 wCur  = alpha / (1.0f + Luma(cur));
        const f32 wHist = (1.0f - alpha) / (1.0f + Luma(hist));
        const glm::vec3 res = (cur * wCur + hist * wHist) / std::max(wCur + wHist, 1e-6f);
        // La caja YCoCg incluye colores fuera de gamut: la historia clipeada puede
        // traer canales negativos.
        return glm::max(res, glm::vec3(0.0f));
    }

    TaaBlock MakeTaaBlock(const FrameView& view, const TaaSettings& settings, bool reset) {
        TaaBlock b {};
        b.invViewProjNoJitter  = glm::inverse(view.viewProjNoJitter);
        b.prevViewProjNoJitter = view.prevViewProjNoJitter;
        b.jitterUv = glm::vec4(view.jitterNdc * 0.5f, 0.0f, 0.0f);

        const f32 w = static_cast<f32>(view.width);
        const f32 h = static_cast<f32>(view.height);
        b.screen = glm::vec4(w, h, w > 0.0f ? 1.0f / w : 0.0f, h > 0.0f ? 1.0f / h : 0.0f);

        const bool sinHistoria = reset || !view.historyValid;
        const f32  alfa = sinHistoria ? 1.0f : std::clamp(settings.alpha, kTaaMinAlpha, 1.0f);
        b.params = glm::vec4(alfa, settings.debugVelocity ? 1.0f : 0.0f, 0.0f, 0.0f);
        return b;
    }

}
}
