#include "Math.h"

namespace efengine {
namespace math {

    std::optional<glm::vec2> ProjectToScreen(const glm::mat4& viewProj,
                                             const glm::vec3& world,
                                             const ScreenRect& rect) {
        const glm::vec4 clip = viewProj * glm::vec4(world, 1.0f);

        // w es la profundidad en espacio de camara. El epsilon y no == 0.0f
        // porque un punto exactamente sobre el plano de la camara da w = 0 y
        // dividir por el da inf.
        if (clip.w <= 1e-6f) return std::nullopt;

        const f32 ndcX = clip.x / clip.w;
        const f32 ndcY = clip.y / clip.w;

        // NDC va de -1 a 1 con la Y hacia arriba; pantalla va de 0 a w/h con la
        // Y hacia abajo. De ahi el (0.5 - ndcY * 0.5) en vez de (ndcY * 0.5 + 0.5).
        return glm::vec2(rect.x + (ndcX * 0.5f + 0.5f) * rect.w,
                         rect.y + (0.5f - ndcY * 0.5f) * rect.h);
    }

}
}
