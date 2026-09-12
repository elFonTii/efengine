#include "efengine/gameplay/CollisionMesh.h"

namespace efengine {
namespace gameplay {

    void AppendSubmesh(CollisionMesh& out, const std::vector<glm::vec3>& positions,
                       const std::vector<u32>& indices) {
        if (positions.empty() || indices.empty()) return;

        const u32 base = static_cast<u32>(out.positions.size() / 3u);

        out.positions.reserve(out.positions.size() + positions.size() * 3u);
        for (const glm::vec3& p : positions) {
            out.positions.push_back(p.x);
            out.positions.push_back(p.y);
            out.positions.push_back(p.z);
        }

        out.indices.reserve(out.indices.size() + indices.size());
        for (const u32 i : indices) out.indices.push_back(base + i);
    }

}
}
