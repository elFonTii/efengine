#include "Cull.h"

namespace efengine {
namespace renderer {

    void AppendMeshSpan(u32 item, u32 mesh, const AABB& local, const glm::mat4& world,
                        std::vector<MeshSpan>& out) {
        if (!local.Valid()) return;

        MeshSpan span;
        span.item   = item;
        span.mesh   = mesh;
        span.bounds = local.Transformed(world);
        out.push_back(span);
    }

    bool Overlaps(const AABB& a, const AABB& b) {
        if (!a.Valid() || !b.Valid()) return false;
        return a.min.x <= b.max.x && a.max.x >= b.min.x
            && a.min.y <= b.max.y && a.max.y >= b.min.y
            && a.min.z <= b.max.z && a.max.z >= b.min.z;
    }

    void CullAabb(const std::vector<MeshSpan>& spans, const AABB& volume,
                  std::vector<u32>& out) {
        out.clear();
        for (u32 i = 0u; i < static_cast<u32>(spans.size()); ++i) {
            if (Overlaps(spans[i].bounds, volume)) out.push_back(i);
        }
    }

}
}
