#include <efengine/renderer/Bounds.h>

#include <limits>

namespace efengine {
namespace renderer {

    bool AABB::Valid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    glm::vec3 AABB::Center() const {
        return (min + max) * 0.5f;
    }

    glm::vec3 AABB::Extents() const {
        return (max - min) * 0.5f;
    }

    f32 AABB::Radius() const {
        return glm::length(Extents());
    }

    AABB AABB::Transformed(const glm::mat4& m) const {
        const glm::vec3 center  = glm::vec3(m * glm::vec4(Center(), 1.0f));
        const glm::vec3 extents = Extents();

        glm::vec3 newExtents { 0.0f };
        for (i32 row = 0; row < 3; ++row) {
            for (i32 col = 0; col < 3; ++col) {
                newExtents[row] += glm::abs(m[col][row]) * extents[col];
            }
        }

        AABB out;
        out.min = center - newExtents;
        out.max = center + newExtents;
        return out;
    }

    AABB AABB::Merge(const AABB& a, const AABB& b) {
        AABB out;
        out.min = glm::min(a.min, b.min);
        out.max = glm::max(a.max, b.max);
        return out;
    }

    AABB AABB::Empty() {
        const f32 inf = std::numeric_limits<f32>::infinity();
        AABB out;
        out.min = glm::vec3( inf);
        out.max = glm::vec3(-inf);
        return out;
    }

    AABB ComputeBounds(const std::vector<Vertex>& vertices) {
        AABB out = AABB::Empty();
        for (const Vertex& v : vertices) {
            out.min = glm::min(out.min, v.position);
            out.max = glm::max(out.max, v.position);
        }
        return out;
    }

}
}
