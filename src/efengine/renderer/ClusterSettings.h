#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    struct ClusterSettings {
        u32 tileSizePx          = 64u;
        u32 slices              = 32u;
        u32 maxLightsPerCluster = 128u;
        f32 nearSplit           = 1.0f;      // m: el corte 0 va del near de camara hasta aca
        f32 farLimit            = 1000.0f;   // m: techo de los cortes exponenciales
        u32 debugView           = 0u;        // 0 = imagen, 1 = heatmap, 2 = cortes Z
    };

}
}
