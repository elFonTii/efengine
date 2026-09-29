#include "efengine/renderer/DdgiGridFit.h"

#include <algorithm>

namespace efengine {
namespace renderer {

    namespace {
        constexpr f32 kMargen        = 0.9f;
        constexpr f32 kSpacingMinimo = 0.1f;

        f32 SpacingDeEje(f32 largo, i32 n) {
            if (n <= 1) return 1.0f;
            return std::max(largo / static_cast<f32>(n - 1), kSpacingMinimo);
        }
    }

    bool FitDdgiGridToBounds(DdgiGrid& grid, const AABB& bounds) {
        if (!bounds.Valid()) return false;

        const glm::vec3 ext = bounds.Extents() * kMargen;
        grid.origin  = bounds.Center() - ext;
        grid.spacing = glm::vec3(SpacingDeEje(2.0f * ext.x, grid.counts.x),
                                 SpacingDeEje(2.0f * ext.y, grid.counts.y),
                                 SpacingDeEje(2.0f * ext.z, grid.counts.z));
        return true;
    }

}
}
