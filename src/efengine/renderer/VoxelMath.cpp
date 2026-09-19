#include "efengine/renderer/VoxelMath.h"

#include <algorithm>
#include <cmath>

namespace efengine {
namespace renderer {

    VoxelGridDesc FitVoxelGrid(const AABB& worldBounds, u32 resolution) {
        VoxelGridDesc g;
        g.resolution = std::max(1u, resolution);

        if (!worldBounds.Valid()) return g;   // origin (0,0,0), voxelSize 1

        // Extents() devuelve el semi-tamano de la caja: se duplica para
        // obtener el lado completo.
        const glm::vec3 extents = 2.0f * worldBounds.Extents();
        const f32 lado = std::max({ extents.x, extents.y, extents.z });
        if (lado <= 0.0f) return g;

        g.voxelSize = lado / static_cast<f32>(g.resolution);
        g.origin    = worldBounds.Center() - glm::vec3(0.5f * lado);
        return g;
    }

    glm::vec3 VoxelToWorld(const VoxelGridDesc& g, glm::ivec3 voxel) {
        return g.origin + (glm::vec3(voxel) + 0.5f) * g.voxelSize;
    }

    glm::ivec3 WorldToVoxel(const VoxelGridDesc& g, glm::vec3 world) {
        const glm::vec3 local = (world - g.origin) / g.voxelSize;
        return glm::ivec3(std::floor(local.x), std::floor(local.y), std::floor(local.z));
    }

    bool InsideGrid(const VoxelGridDesc& g, glm::ivec3 voxel) {
        const i32 r = static_cast<i32>(g.resolution);
        return voxel.x >= 0 && voxel.x < r
            && voxel.y >= 0 && voxel.y < r
            && voxel.z >= 0 && voxel.z < r;
    }

    AABB GridBounds(const VoxelGridDesc& g) {
        return AABB{ g.origin, g.origin + glm::vec3(GridExtent(g)) };
    }

    f32 GridExtent(const VoxelGridDesc& g) {
        return static_cast<f32>(g.resolution) * g.voxelSize;
    }

}
}
