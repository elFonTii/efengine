#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Frustum.h>
#include <efengine/renderer/Light.h>
#include <efengine/renderer/LightMath.h>
#include <efengine/renderer/ShaderBlocks.h>

#include <vector>

namespace efengine {
namespace renderer {

    // Lo que LightUploadPass sube por frame. PackLights limpia los vectores sin
    // liberar capacidad: el pase lo tiene como miembro y lo reusa.
    struct PackedLights {
        LightsBlock           block {};
        std::vector<GpuLight> locals;
        std::vector<u32>      visible;            // indices en 'locals'
        u32 totalLocals      = 0u;                // antes del recorte
        u32 totalDirectional = 0u;                // antes del recorte
    };

    GpuLight PackGpuLight(const Light& light);

    // La esfera del rango para Point, la del cono para Spot.
    BoundingSphere LocalLightBounds(const Light& light);

    // Direccionales: el PrimarySun primero y el resto en orden, hasta
    // kMaxDirectionalLights. Locales: en orden hasta kMaxLocalLights, con
    // 'visible' = las que tocan el frustum.
    void PackLights(const std::vector<Light>& lights, const Frustum& frustum, PackedLights& out);

}
}
