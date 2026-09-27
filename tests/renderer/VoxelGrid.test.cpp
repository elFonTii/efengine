// memoryBytes() es lo que muestran el panel de DDGI y el log de la
// voxelizacion. Un VoxelGrid por defecto no toca GL: su desc es el default.
#include <doctest/doctest.h>
#include <efengine/renderer/VoxelGrid.h>

using efengine::renderer::VoxelGrid;
using efengine::renderer::kVoxelResolution;

TEST_CASE("VoxelGrid: la memoria cuenta albedo, normal y emision") {
    const VoxelGrid g;
    const u64 n = kVoxelResolution;
    CHECK(g.memoryBytes() == n * n * n * (4u + 2u + 4u));   // RGBA8 + RG8 + RGBA8
}
