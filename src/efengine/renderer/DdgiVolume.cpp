#include "DdgiVolume.h"

#include <efengine/core/Assert.h>

#include <algorithm>

namespace efengine {
namespace renderer {

    DdgiGrid SanitizeGrid(const DdgiGrid& grid) {
        DdgiGrid out = grid;

        out.counts.x = std::clamp(grid.counts.x, 1, kMaxProbesPerAxis);
        out.counts.y = std::clamp(grid.counts.y, 1, kMaxProbesPerAxis);
        out.counts.z = std::clamp(grid.counts.z, 1, kMaxProbesPerAxis);

        // Spacing cero o negativo hace que (worldPos - origin) / spacing sea
        // inf/NaN en el shader y la GI desaparezca sin error visible.
        const f32 kMinSpacing = 0.01f;
        out.spacing.x = std::max(grid.spacing.x, kMinSpacing);
        out.spacing.y = std::max(grid.spacing.y, kMinSpacing);
        out.spacing.z = std::max(grid.spacing.z, kMinSpacing);

        return out;
    }

    u32 ProbeCount(const DdgiGrid& grid) {
        if (grid.counts.x <= 0 || grid.counts.y <= 0 || grid.counts.z <= 0) return 0u;
        return static_cast<u32>(grid.counts.x)
             * static_cast<u32>(grid.counts.y)
             * static_cast<u32>(grid.counts.z);
    }

    glm::ivec3 ProbeCoords(const DdgiGrid& grid, u32 index) {
        EF_ASSERT(index < ProbeCount(grid), "ProbeCoords: indice de probe fuera de rango");

        const u32 cx = static_cast<u32>(grid.counts.x);
        const u32 cy = static_cast<u32>(grid.counts.y);

        const u32 x = index % cx;
        const u32 y = (index / cx) % cy;
        const u32 z = index / (cx * cy);

        return glm::ivec3(static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(z));
    }

    u32 ProbeIndex(const DdgiGrid& grid, glm::ivec3 coords) {
        EF_ASSERT(coords.x >= 0 && coords.x < grid.counts.x
               && coords.y >= 0 && coords.y < grid.counts.y
               && coords.z >= 0 && coords.z < grid.counts.z,
                  "ProbeIndex: coordenadas de probe fuera de la grilla");

        return static_cast<u32>(coords.x)
             + static_cast<u32>(grid.counts.x)
             * (static_cast<u32>(coords.y)
                + static_cast<u32>(grid.counts.y) * static_cast<u32>(coords.z));
    }

    glm::vec3 ProbeWorldPosition(const DdgiGrid& grid, u32 index) {
        const glm::ivec3 c = ProbeCoords(grid, index);
        return grid.origin + glm::vec3(c) * grid.spacing;
    }

    glm::ivec2 AtlasTileCount(const DdgiGrid& grid) {
        return glm::ivec2(grid.counts.x * grid.counts.y, grid.counts.z);
    }

    glm::ivec2 AtlasTileCoords(const DdgiGrid& grid, u32 index) {
        const glm::ivec3 c = ProbeCoords(grid, index);
        // Columna = plano XY aplanado; fila = Z. Coincide con
        // tileX = index % (countX*countY) por construccion del indice.
        return glm::ivec2(c.x + grid.counts.x * c.y, c.z);
    }

    glm::ivec2 IrradianceAtlasSize(const DdgiGrid& grid) {
        return AtlasTileCount(grid) * static_cast<i32>(kIrradianceTileBordered);
    }

    glm::ivec2 DistanceAtlasSize(const DdgiGrid& grid) {
        return AtlasTileCount(grid) * static_cast<i32>(kDistanceTileBordered);
    }

    UpdateRange NextRange(u32 cursor, u32 perFrame, u32 total) {
        UpdateRange r;
        if (total == 0u || perFrame == 0u) {
            r.first      = (total == 0u) ? 0u : cursor % total;
            r.count      = 0u;
            r.nextCursor = (total == 0u) ? 0u : cursor % total;
            return r;
        }

        r.first      = cursor % total;
        r.count      = std::min(perFrame, total);
        r.nextCursor = (r.first + r.count) % total;
        return r;
    }

}
}
