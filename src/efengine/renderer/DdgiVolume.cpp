#include "DdgiVolume.h"

#include <efengine/core/Assert.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace efengine {
namespace renderer {

    namespace {
        u32 Pcg(u32 v) {
            const u32 state = v * 747796405u + 2891336453u;
            const u32 word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
            return (word >> 22u) ^ word;
        }

        f32 ToUnit(u32 h) { return static_cast<f32>(h >> 8u) * (1.0f / 16777216.0f); }
    }

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

    usize ProbeDataBytes(const DdgiGrid& grid) {
        return static_cast<usize>(ProbeCount(grid)) * sizeof(glm::vec4);
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

    usize ProbeStateBytes(const DdgiGrid& grid) {
        return static_cast<usize>(ProbeCount(grid)) * sizeof(glm::uvec4);
    }

    f32 DistanceClamp(const DdgiGrid& grid) {
        return 1.5f * glm::length(SanitizeGrid(grid).spacing);
    }

    glm::vec3 FibonacciDirection(u32 i, u32 count) {
        EF_ASSERT(count > 0u, "FibonacciDirection: count cero");
        constexpr f32 kB = 0.618033988749895f;   // proporcion aurea - 1
        const f32 fi       = static_cast<f32>(i);
        const f32 x        = fi * kB;
        const f32 phi      = 2.0f * glm::pi<f32>() * (x - std::floor(x));
        const f32 cosTheta = 1.0f - (2.0f * fi + 1.0f) / static_cast<f32>(count);
        const f32 sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
        return glm::vec3(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
    }

    glm::mat3 RandomRayRotation(u32 seed) {
        const u32 h1 = Pcg(seed);
        const u32 h2 = Pcg(h1);
        const u32 h3 = Pcg(h2);
        const f32 u1 = ToUnit(h1);
        const f32 u2 = ToUnit(h2);
        const f32 u3 = ToUnit(h3);

        const f32 a     = std::sqrt(1.0f - u1);
        const f32 b     = std::sqrt(u1);
        const f32 dosPi = 2.0f * glm::pi<f32>();
        const glm::quat q(b * std::cos(dosPi * u3),    // w
                          a * std::sin(dosPi * u2),
                          a * std::cos(dosPi * u2),
                          b * std::sin(dosPi * u3));
        return glm::mat3_cast(q);
    }

    f32 EffectiveHysteresis(u32 age, f32 hMax) {
        if (age < 2u) return 0.0f;
        const f32 progresivo = static_cast<f32>(age - 1u) / static_cast<f32>(age);
        return std::min(std::clamp(hMax, 0.0f, 1.0f), progresivo);
    }

}
}
