#include "efengine/renderer/ClusterMath.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>

namespace efengine {
namespace renderer {

    ClusterGrid MakeClusterGrid(const ClusterSettings& s, u32 width, u32 height,
                                f32 cameraNear, f32 cameraFar) {
        ClusterGrid g;
        g.tileSizePx          = std::clamp(s.tileSizePx, kMinClusterTilePx, kMaxClusterTilePx);
        g.slices              = std::clamp(s.slices, kMinClusterSlices, kMaxClusterSlices);
        g.maxLightsPerCluster = std::clamp(s.maxLightsPerCluster, kMinLightsPerCluster, kMaxLightsPerCluster);

        const u32 w = std::max(width, 1u);
        const u32 h = std::max(height, 1u);
        g.tilesX = (w + g.tileSizePx - 1u) / g.tileSizePx;
        g.tilesY = (h + g.tileSizePx - 1u) / g.tileSizePx;

        g.cameraNear = std::max(cameraNear, 1e-4f);
        g.cameraFar  = std::max(cameraFar, g.cameraNear * 2.0f);
        g.nearSplit  = std::max(s.nearSplit, g.cameraNear);
        g.farLimit   = std::max(std::min(g.cameraFar, s.farLimit), 2.0f * g.nearSplit);
        return g;
    }

    bool SameGrid(const ClusterGrid& a, const ClusterGrid& b) {
        return a.tilesX == b.tilesX && a.tilesY == b.tilesY && a.slices == b.slices
            && a.tileSizePx == b.tileSizePx && a.maxLightsPerCluster == b.maxLightsPerCluster
            && a.cameraNear == b.cameraNear && a.cameraFar == b.cameraFar
            && a.nearSplit == b.nearSplit && a.farLimit == b.farLimit;
    }

    u32 ClusterSlice(const ClusterGrid& g, f32 viewZ) {
        if (!(viewZ >= g.nearSplit)) return 0u;
        const f32 escala = static_cast<f32>(g.slices - 1u) / std::log(g.farLimit / g.nearSplit);
        const f32 s = std::floor((std::log(viewZ) - std::log(g.nearSplit)) * escala);
        const u32 k = 1u + static_cast<u32>(std::min(std::max(s, 0.0f), static_cast<f32>(g.slices)));
        return std::min(k, g.slices - 1u);
    }

    f32 SliceNear(const ClusterGrid& g, u32 k) {
        if (k == 0u) return g.cameraNear;
        const f32 t = static_cast<f32>(k - 1u) / static_cast<f32>(g.slices - 1u);
        return std::min(g.nearSplit * std::pow(g.farLimit / g.nearSplit, t), g.cameraFar);
    }

    f32 SliceFar(const ClusterGrid& g, u32 k) {
        if (k + 1u >= g.slices) return std::max(g.cameraFar, SliceNear(g, k));
        return SliceNear(g, k + 1u);
    }

    u32 ClusterIndex(const ClusterGrid& g, u32 tx, u32 ty, u32 slice) {
        return (slice * g.tilesY + ty) * g.tilesX + tx;
    }

    std::vector<ClusterAabb> BuildClusterAabbs(const ClusterGrid& g, const glm::mat4& inverseProjection,
                                               u32 width, u32 height) {
        std::vector<ClusterAabb> out(g.Count());
        const f32 w = static_cast<f32>(std::max(width, 1u));
        const f32 h = static_cast<f32>(std::max(height, 1u));
        const f32 t = static_cast<f32>(g.tileSizePx);

        // Rayo por un pixel, escalado a z = -1: multiplicarlo por z da el punto.
        auto rayo = [&](f32 px, f32 py) {
            glm::vec4 v = inverseProjection * glm::vec4(2.0f * px / w - 1.0f, 2.0f * py / h - 1.0f, -1.0f, 1.0f);
            v /= v.w;
            return glm::vec3(v) / -v.z;
        };

        const f32 inf = std::numeric_limits<f32>::infinity();
        for (u32 k = 0u; k < g.slices; ++k) {
            const f32 zCerca = SliceNear(g, k);
            const f32 zLejos = SliceFar(g, k);
            for (u32 ty = 0u; ty < g.tilesY; ++ty) {
                const f32 y0 = static_cast<f32>(ty) * t;
                const f32 y1 = std::min(static_cast<f32>(ty + 1u) * t, h);
                for (u32 tx = 0u; tx < g.tilesX; ++tx) {
                    const f32 x0 = static_cast<f32>(tx) * t;
                    const f32 x1 = std::min(static_cast<f32>(tx + 1u) * t, w);

                    glm::vec3 lo(inf), hi(-inf);
                    for (const glm::vec3& r : { rayo(x0, y0), rayo(x1, y0), rayo(x0, y1), rayo(x1, y1) }) {
                        for (f32 z : { zCerca, zLejos }) {
                            lo = glm::min(lo, r * z);
                            hi = glm::max(hi, r * z);
                        }
                    }
                    out[ClusterIndex(g, tx, ty, k)] = ClusterAabb{ glm::vec4(lo, 0.0f), glm::vec4(hi, 0.0f) };
                }
            }
        }
        return out;
    }

}
}
