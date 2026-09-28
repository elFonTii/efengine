#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/ClusterSettings.h>

#include <glm/glm.hpp>
#include <vector>

namespace efengine {
namespace renderer {

    inline constexpr u32 kMinClusterTilePx    = 16u;
    inline constexpr u32 kMaxClusterTilePx    = 256u;
    inline constexpr u32 kMinClusterSlices    = 2u;
    inline constexpr u32 kMaxClusterSlices    = 64u;
    inline constexpr u32 kMinLightsPerCluster = 8u;
    inline constexpr u32 kMaxLightsPerCluster = 512u;

    // La grilla saneada para un target y una camara. Es lo que comparten las
    // cajas (CPU), el compute que las llena y pbr.frag que las lee.
    struct ClusterGrid {
        u32 tilesX = 1u;
        u32 tilesY = 1u;
        u32 slices = kMinClusterSlices;
        u32 tileSizePx = 64u;
        u32 maxLightsPerCluster = 128u;
        f32 cameraNear = 0.1f;
        f32 cameraFar  = 1000.0f;
        f32 nearSplit  = 1.0f;
        f32 farLimit   = 1000.0f;

        u32 Count() const { return tilesX * tilesY * slices; }
    };

    ClusterGrid MakeClusterGrid(const ClusterSettings& s, u32 width, u32 height,
                                f32 cameraNear, f32 cameraFar);

    bool SameGrid(const ClusterGrid& a, const ClusterGrid& b);

    // Mismo calculo que ClusterSlice de common/clusters.glsl. viewZ positiva.
    u32 ClusterSlice(const ClusterGrid& g, f32 viewZ);

    // Bordes del corte k en profundidad de vista positiva: el 0 arranca en el
    // near de camara y el ultimo termina en el far de camara.
    f32 SliceNear(const ClusterGrid& g, u32 k);
    f32 SliceFar (const ClusterGrid& g, u32 k);

    // x, despues y, despues el corte. El mismo orden que common/clusters.glsl.
    u32 ClusterIndex(const ClusterGrid& g, u32 tx, u32 ty, u32 slice);

    // Espacio de VISTA, camara mirando a -Z. vec4 por el layout std430 del SSBO.
    struct ClusterAabb {
        glm::vec4 min;
        glm::vec4 max;
    };

    std::vector<ClusterAabb> BuildClusterAabbs(const ClusterGrid& g, const glm::mat4& inverseProjection,
                                               u32 width, u32 height);

}
}
