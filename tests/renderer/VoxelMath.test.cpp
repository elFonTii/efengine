#include <doctest/doctest.h>
#include <efengine/renderer/VoxelMath.h>

using efengine::renderer::AABB;
using efengine::renderer::FitVoxelGrid;
using efengine::renderer::GridBounds;
using efengine::renderer::GridExtent;
using efengine::renderer::InsideGrid;
using efengine::renderer::VoxelGridDesc;
using efengine::renderer::VoxelToWorld;
using efengine::renderer::WorldToVoxel;
using efengine::renderer::kVoxelResolution;

namespace {
    // La sala de Cornell de TestScene: 8 m de lado.
    AABB cornell() { return AABB{ glm::vec3(-4.0f), glm::vec3(4.0f) }; }
}

TEST_CASE("FitVoxelGrid: un cubo de 8 m a 256 da voxeles de 3,125 cm") {
    const VoxelGridDesc g = FitVoxelGrid(cornell(), kVoxelResolution);

    CHECK(g.resolution == kVoxelResolution);
    CHECK(g.voxelSize == doctest::Approx(8.0f / 256.0f));
    CHECK(g.origin.x == doctest::Approx(-4.0f));
    CHECK(g.origin.y == doctest::Approx(-4.0f));
    CHECK(g.origin.z == doctest::Approx(-4.0f));
}

TEST_CASE("FitVoxelGrid: una caja alargada se encierra en el cubo del eje mayor") {
    // 10 x 2 x 4, centrada en (5, 1, 2).
    const AABB caja { glm::vec3(0.0f), glm::vec3(10.0f, 2.0f, 4.0f) };
    const VoxelGridDesc g = FitVoxelGrid(caja, kVoxelResolution);

    CHECK(GridExtent(g) == doctest::Approx(10.0f));

    // Centrado: el cubo de lado 10 alrededor de (5,1,2) arranca en (0,-4,-3).
    CHECK(g.origin.x == doctest::Approx(0.0f));
    CHECK(g.origin.y == doctest::Approx(-4.0f));
    CHECK(g.origin.z == doctest::Approx(-3.0f));

    // Y la contiene entera, que es el invariante que importa.
    const AABB b = GridBounds(g);
    CHECK(b.min.x <= caja.min.x);  CHECK(b.max.x >= caja.max.x);
    CHECK(b.min.y <= caja.min.y);  CHECK(b.max.y >= caja.max.y);
    CHECK(b.min.z <= caja.min.z);  CHECK(b.max.z >= caja.max.z);
}

TEST_CASE("FitVoxelGrid: una AABB invalida no produce NaN") {
    const VoxelGridDesc g = FitVoxelGrid(AABB::Empty(), kVoxelResolution);

    CHECK(g.voxelSize == doctest::Approx(1.0f));
    CHECK(g.origin.x == doctest::Approx(0.0f));
    CHECK(g.voxelSize == g.voxelSize);   // NaN != NaN
}

TEST_CASE("FitVoxelGrid: resolucion cero se clampea a 1 en vez de dividir por cero") {
    const VoxelGridDesc g = FitVoxelGrid(cornell(), 0u);

    CHECK(g.resolution == 1u);
    CHECK(g.voxelSize == doctest::Approx(8.0f));
}

TEST_CASE("WorldToVoxel y VoxelToWorld son inversas sobre los centros") {
    const VoxelGridDesc g = FitVoxelGrid(cornell(), kVoxelResolution);

    const glm::ivec3 casos[] = {
        glm::ivec3(0, 0, 0),
        glm::ivec3(1, 2, 3),
        glm::ivec3(128, 128, 128),
        glm::ivec3(255, 255, 255),
    };

    for (const glm::ivec3& v : casos) {
        CHECK(WorldToVoxel(g, VoxelToWorld(g, v)) == v);
    }
}

TEST_CASE("VoxelToWorld devuelve el CENTRO, no la esquina") {
    const VoxelGridDesc g = FitVoxelGrid(cornell(), kVoxelResolution);
    const glm::vec3 p = VoxelToWorld(g, glm::ivec3(0, 0, 0));

    CHECK(p.x == doctest::Approx(-4.0f + 0.5f * g.voxelSize));
}

TEST_CASE("InsideGrid rechaza lo que cae fuera del rango") {
    const VoxelGridDesc g = FitVoxelGrid(cornell(), kVoxelResolution);

    CHECK(InsideGrid(g, glm::ivec3(0, 0, 0)));
    CHECK(InsideGrid(g, glm::ivec3(255, 255, 255)));
    CHECK_FALSE(InsideGrid(g, glm::ivec3(-1, 0, 0)));
    CHECK_FALSE(InsideGrid(g, glm::ivec3(256, 0, 0)));
    CHECK_FALSE(InsideGrid(g, glm::ivec3(0, 0, 256)));
}

TEST_CASE("WorldToVoxel fuera del grid da un indice que InsideGrid rechaza") {
    const VoxelGridDesc g = FitVoxelGrid(cornell(), kVoxelResolution);

    CHECK_FALSE(InsideGrid(g, WorldToVoxel(g, glm::vec3(-100.0f, 0.0f, 0.0f))));
    CHECK_FALSE(InsideGrid(g, WorldToVoxel(g, glm::vec3(0.0f, 100.0f, 0.0f))));
}

TEST_CASE("GridBounds arranca en el origen y mide resolution * voxelSize") {
    const VoxelGridDesc g = FitVoxelGrid(cornell(), kVoxelResolution);
    const AABB b = GridBounds(g);

    CHECK(b.min.x == doctest::Approx(g.origin.x));
    CHECK(b.max.x == doctest::Approx(g.origin.x + GridExtent(g)));
    CHECK(GridExtent(g) == doctest::Approx(8.0f));
}
