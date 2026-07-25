#include "Aabb.h"

namespace efengine {
namespace math {

    bool AabbIsEmpty(const Aabb& box) {
        return box.min.x > box.max.x || box.min.y > box.max.y || box.min.z > box.max.z;
    }

    Aabb AabbFromPoints(const glm::vec3* points, usize count) {
        if (points == null || count == 0) return Aabb{};

        Aabb box { points[0], points[0] };
        for (usize i = 1; i < count; ++i) {
            box.min = glm::min(box.min, points[i]);
            box.max = glm::max(box.max, points[i]);
        }
        return box;
    }

    Aabb MergeAabb(const Aabb& a, const Aabb& b) {
        if (AabbIsEmpty(a)) return b;
        if (AabbIsEmpty(b)) return a;
        return Aabb { glm::min(a.min, b.min), glm::max(a.max, b.max) };
    }

    Aabb TransformAabb(const glm::mat4& m, const Aabb& box) {
        if (AabbIsEmpty(box)) return box;

        const glm::vec3 corners[8] = {
            { box.min.x, box.min.y, box.min.z },
            { box.max.x, box.min.y, box.min.z },
            { box.min.x, box.max.y, box.min.z },
            { box.max.x, box.max.y, box.min.z },
            { box.min.x, box.min.y, box.max.z },
            { box.max.x, box.min.y, box.max.z },
            { box.min.x, box.max.y, box.max.z },
            { box.max.x, box.max.y, box.max.z },
        };

        glm::vec3 transformed[8];
        for (usize i = 0; i < 8; ++i) {
            transformed[i] = glm::vec3(m * glm::vec4(corners[i], 1.0f));
        }
        return AabbFromPoints(transformed, 8);
    }

    glm::vec3 AabbCenter(const Aabb& box)  { return (box.min + box.max) * 0.5f; }
    glm::vec3 AabbExtents(const Aabb& box) { return (box.max - box.min) * 0.5f; }

}
}
