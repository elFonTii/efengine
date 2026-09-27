#include "efengine/renderer/Frustum.h"

namespace efengine {
namespace renderer {

    Frustum ExtractFrustum(const glm::mat4& m) {
        // glm es column-major: la fila i es (m[0][i], m[1][i], m[2][i], m[3][i]).
        const glm::vec4 r0(m[0][0], m[1][0], m[2][0], m[3][0]);
        const glm::vec4 r1(m[0][1], m[1][1], m[2][1], m[3][1]);
        const glm::vec4 r2(m[0][2], m[1][2], m[2][2], m[3][2]);
        const glm::vec4 r3(m[0][3], m[1][3], m[2][3], m[3][3]);

        Frustum f;
        f.planes[0] = r3 + r0;
        f.planes[1] = r3 - r0;
        f.planes[2] = r3 + r1;
        f.planes[3] = r3 - r1;
        f.planes[4] = r3 + r2;   // clip z de GL en [-w, w]
        f.planes[5] = r3 - r2;
        for (glm::vec4& p : f.planes) {
            const f32 largo = glm::length(glm::vec3(p));
            if (largo > 0.0f) p /= largo;
        }
        return f;
    }

    bool SphereInFrustum(const Frustum& f, const BoundingSphere& s) {
        for (const glm::vec4& p : f.planes) {
            if (glm::dot(glm::vec3(p), s.center) + p.w < -s.radius) return false;
        }
        return true;
    }

}
}
