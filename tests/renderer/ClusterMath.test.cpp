#include <doctest/doctest.h>
#include <efengine/renderer/ClusterMath.h>
#include <efengine/renderer/LightMath.h>
#include <efengine/renderer/ShaderBlocks.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <random>
#include <vector>

using namespace efengine;
using namespace efengine::renderer;

namespace {
    ClusterGrid GrillaHd() {
        return MakeClusterGrid(ClusterSettings{}, 1920u, 1080u, 0.1f, 5000.0f);
    }

    // Punto de vista por el pixel (px, py) a profundidad positiva z.
    glm::vec3 PuntoDeVista(const glm::mat4& invProj, f32 px, f32 py, f32 w, f32 h, f32 z) {
        glm::vec4 v = invProj * glm::vec4(2.0f * px / w - 1.0f, 2.0f * py / h - 1.0f, -1.0f, 1.0f);
        v /= v.w;
        return glm::vec3(v) / -v.z * z;
    }

    u32 ClusterDelPixel(const ClusterGrid& g, f32 px, f32 py, f32 z) {
        const u32 tx = std::min(static_cast<u32>(px / static_cast<f32>(g.tileSizePx)), g.tilesX - 1u);
        const u32 ty = std::min(static_cast<u32>(py / static_cast<f32>(g.tileSizePx)), g.tilesY - 1u);
        return ClusterIndex(g, tx, ty, ClusterSlice(g, z));
    }
}

TEST_CASE("MakeClusterGrid: 1080p con los defaults da 30 x 17 x 32") {
    const ClusterGrid g = GrillaHd();
    CHECK(g.tilesX == 30u);
    CHECK(g.tilesY == 17u);
    CHECK(g.slices == 32u);
    CHECK(g.Count() == 16320u);
    CHECK(g.nearSplit == doctest::Approx(1.0f));
    CHECK(g.farLimit  == doctest::Approx(1000.0f));
}

TEST_CASE("MakeClusterGrid: un tile parcial cuenta entero") {
    const ClusterGrid g = MakeClusterGrid(ClusterSettings{}, 1281u, 721u, 0.1f, 100.0f);
    CHECK(g.tilesX == 21u);
    CHECK(g.tilesY == 12u);
}

TEST_CASE("MakeClusterGrid: sanea settings fuera de rango") {
    ClusterSettings chico;
    chico.tileSizePx = 1u; chico.slices = 1u; chico.maxLightsPerCluster = 0u;
    const ClusterGrid a = MakeClusterGrid(chico, 1920u, 1080u, 0.1f, 100.0f);
    CHECK(a.tileSizePx == kMinClusterTilePx);
    CHECK(a.slices == kMinClusterSlices);
    CHECK(a.maxLightsPerCluster == kMinLightsPerCluster);

    ClusterSettings grande;
    grande.tileSizePx = 10000u; grande.slices = 1000u; grande.maxLightsPerCluster = 100000u;
    const ClusterGrid b = MakeClusterGrid(grande, 1920u, 1080u, 0.1f, 100.0f);
    CHECK(b.tileSizePx == kMaxClusterTilePx);
    CHECK(b.slices == kMaxClusterSlices);
    CHECK(b.maxLightsPerCluster == kMaxLightsPerCluster);
}

TEST_CASE("MakeClusterGrid: farLimit nunca queda por debajo de 2x nearSplit") {
    const ClusterGrid g = MakeClusterGrid(ClusterSettings{}, 640u, 480u, 0.1f, 1.5f);
    CHECK(g.farLimit == doctest::Approx(2.0f));
    for (u32 k = 0u; k < g.slices; ++k) {
        CAPTURE(k);
        CHECK(SliceNear(g, k) <= SliceFar(g, k));
        CHECK(std::isfinite(SliceFar(g, k)));
    }
}

TEST_CASE("MakeClusterGrid: target de tamano cero no divide por cero") {
    const ClusterGrid g = MakeClusterGrid(ClusterSettings{}, 0u, 0u, 0.1f, 100.0f);
    CHECK(g.tilesX == 1u);
    CHECK(g.tilesY == 1u);
}

TEST_CASE("MakeClusterGrid: planos de camara extremos dan cortes finitos y monotonos") {
    const ClusterGrid g = MakeClusterGrid(ClusterSettings{}, 1920u, 1080u, 0.01f, 20000.0f);
    f32 anterior = 0.0f;
    for (u32 k = 0u; k < g.slices; ++k) {
        CAPTURE(k);
        CHECK(std::isfinite(SliceNear(g, k)));
        CHECK(SliceNear(g, k) >= anterior);
        anterior = SliceNear(g, k);
    }
    CHECK(SliceFar(g, g.slices - 1u) == doctest::Approx(20000.0f));
}

TEST_CASE("SameGrid: detecta cambios de tamano y de camara") {
    const ClusterGrid a = GrillaHd();
    ClusterGrid b = a;
    CHECK(SameGrid(a, b));
    b.tilesX += 1u;
    CHECK_FALSE(SameGrid(a, b));
    const ClusterGrid c = MakeClusterGrid(ClusterSettings{}, 1920u, 1080u, 0.1f, 4000.0f);
    CHECK_FALSE(SameGrid(a, c));
}

TEST_CASE("ClusterSlice: lo que esta antes de nearSplit cae en el corte 0") {
    const ClusterGrid g = GrillaHd();
    CHECK(ClusterSlice(g, 0.1f) == 0u);
    CHECK(ClusterSlice(g, 0.5f) == 0u);
    CHECK(ClusterSlice(g, 0.999f) == 0u);
}

TEST_CASE("ClusterSlice: nearSplit abre el corte 1 y farLimit cae en el ultimo") {
    const ClusterGrid g = GrillaHd();
    CHECK(ClusterSlice(g, 1.0f) == 1u);
    CHECK(ClusterSlice(g, 1000.0f) == 31u);
    CHECK(ClusterSlice(g, 4000.0f) == 31u);
}

TEST_CASE("ClusterSlice: es monotona") {
    const ClusterGrid g = GrillaHd();
    u32 anterior = 0u;
    for (f32 z = 0.1f; z < 5000.0f; z *= 1.05f) {
        const u32 k = ClusterSlice(g, z);
        CHECK(k >= anterior);
        anterior = k;
    }
}

TEST_CASE("ClusterSlice: el borde de SliceNear cae en el corte que abre") {
    const ClusterGrid g = GrillaHd();
    for (u32 k = 1u; k < g.slices; ++k) {
        CAPTURE(k);
        CHECK(ClusterSlice(g, SliceNear(g, k) * 1.0001f) == k);
        if (k >= 2u) CHECK(ClusterSlice(g, SliceNear(g, k) * 0.9999f) == k - 1u);
    }
}

TEST_CASE("SliceNear/SliceFar: cubren de near a far sin huecos") {
    const ClusterGrid g = GrillaHd();
    CHECK(SliceNear(g, 0u) == doctest::Approx(0.1f));
    CHECK(SliceFar(g, g.slices - 1u) == doctest::Approx(5000.0f));
    for (u32 k = 0u; k + 1u < g.slices; ++k) CHECK(SliceFar(g, k) == SliceNear(g, k + 1u));
}

TEST_CASE("ClusterIndex: es una biyeccion sobre 0..Count-1") {
    const ClusterGrid g = GrillaHd();
    std::vector<bool> visto(g.Count(), false);
    for (u32 k = 0u; k < g.slices; ++k)
        for (u32 ty = 0u; ty < g.tilesY; ++ty)
            for (u32 tx = 0u; tx < g.tilesX; ++tx) {
                const u32 i = ClusterIndex(g, tx, ty, k);
                REQUIRE(i < g.Count());
                CHECK_FALSE(visto[i]);
                visto[i] = true;
            }
}

TEST_CASE("BuildClusterAabbs: cada punto del frustum cae en la caja de su cluster") {
    const glm::mat4 proj = glm::perspective(glm::radians(60.0f), 1920.0f / 1080.0f, 0.1f, 500.0f);
    const glm::mat4 inv  = glm::inverse(proj);
    const ClusterGrid g  = MakeClusterGrid(ClusterSettings{}, 1920u, 1080u, 0.1f, 500.0f);
    const std::vector<ClusterAabb> cajas = BuildClusterAabbs(g, inv, 1920u, 1080u);
    REQUIRE(cajas.size() == g.Count());

    for (f32 px : { 0.5f, 100.3f, 960.0f, 1919.5f })
        for (f32 py : { 0.5f, 540.0f, 1079.5f })
            for (f32 z : { 0.15f, 0.9f, 1.0f, 3.7f, 42.0f, 499.0f }) {
                CAPTURE(px); CAPTURE(py); CAPTURE(z);
                const glm::vec3 p = PuntoDeVista(inv, px, py, 1920.0f, 1080.0f, z);
                const ClusterAabb& c = cajas[ClusterDelPixel(g, px, py, z)];
                const f32 tol = 1e-3f * std::max(1.0f, z);
                CHECK(p.x >= c.min.x - tol); CHECK(p.x <= c.max.x + tol);
                CHECK(p.y >= c.min.y - tol); CHECK(p.y <= c.max.y + tol);
                CHECK(p.z >= c.min.z - tol); CHECK(p.z <= c.max.z + tol);
            }
}

TEST_CASE("BuildClusterAabbs: ninguna caja esta invertida") {
    const glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 500.0f);
    const ClusterGrid g  = MakeClusterGrid(ClusterSettings{}, 1280u, 720u, 0.1f, 500.0f);
    for (const ClusterAabb& c : BuildClusterAabbs(g, glm::inverse(proj), 1280u, 720u)) {
        CHECK(c.min.x <= c.max.x);
        CHECK(c.min.y <= c.max.y);
        CHECK(c.min.z <= c.max.z);
    }
}

TEST_CASE("Culling de clusters: ninguna luz que ilumina un punto queda afuera de su cluster") {
    const glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 300.0f);
    const glm::mat4 inv  = glm::inverse(proj);
    const ClusterGrid g  = MakeClusterGrid(ClusterSettings{}, 1280u, 720u, 0.1f, 300.0f);
    const std::vector<ClusterAabb> cajas = BuildClusterAabbs(g, inv, 1280u, 720u);

    std::mt19937 rng(7u);
    std::uniform_real_distribution<f32> u(0.0f, 1.0f);
    auto ruido = [&]() { return glm::vec3(u(rng) - 0.5f, u(rng) - 0.5f, u(rng) - 0.5f); };

    u32 iluminados = 0u;
    for (int n = 0; n < 4000; ++n) {
        const f32 px = u(rng) * 1280.0f;
        const f32 py = u(rng) * 720.0f;
        const f32 z  = 0.1f + u(rng) * u(rng) * 60.0f;
        const glm::vec3 p = PuntoDeVista(inv, px, py, 1280.0f, 720.0f, z);

        const bool      spot     = u(rng) < 0.5f;
        const f32       range    = 0.5f + 9.5f * u(rng);
        const glm::vec3 pos      = p + ruido() * (2.0f * range);
        const glm::vec3 haciaP   = p - pos;
        const glm::vec3 dir      = glm::normalize(haciaP + ruido() * 2.0f + glm::vec3(0.0f, 0.0f, 1e-3f));
        const f32       cosOuter = std::cos(glm::radians(5.0f + 80.0f * u(rng)));

        const f32  d = glm::length(haciaP);
        const bool ilumina = d < range * 0.999f
                          && (!spot || (d > 1e-4f && glm::dot(dir, haciaP / d) > cosOuter + 1e-3f));
        if (!ilumina) continue;
        ++iluminados;

        const ClusterAabb& c = cajas[ClusterDelPixel(g, px, py, z)];
        CHECK(LocalLightTouchesAabb(spot ? LightType::Spot : LightType::Point, pos, dir, range, cosOuter,
                                    glm::vec3(c.min), glm::vec3(c.max)));
    }
    CHECK(iluminados > 500u);
}

TEST_CASE("Layout std140: ClusterBlock mide 48 y sus campos caen donde el shader los lee") {
    CHECK(sizeof(ClusterBlock) == 48u);
    CHECK(offsetof(ClusterBlock, dims)    ==  0u);
    CHECK(offsetof(ClusterBlock, zParams) == 16u);
    CHECK(offsetof(ClusterBlock, screen)  == 32u);
}

TEST_CASE("MakeClusterBlock: la formula del shader da el mismo corte que ClusterSlice") {
    const ClusterGrid  g = GrillaHd();
    const ClusterBlock b = MakeClusterBlock(g, 2u);
    CHECK(b.dims == glm::uvec4(30u, 17u, 32u, 128u));
    CHECK(b.screen.x == doctest::Approx(64.0f));
    CHECK(b.screen.y == doctest::Approx(2.0f));
    for (f32 z = 0.2f; z < 5000.0f; z *= 1.3f) {
        CAPTURE(z);
        u32 shader = 0u;
        if (z >= b.zParams.x) {
            const f32 s = std::floor((std::log(z) - b.zParams.w) * b.zParams.z);
            shader = std::min(1u + static_cast<u32>(std::min(std::max(s, 0.0f), static_cast<f32>(b.dims.z))),
                              b.dims.z - 1u);
        }
        CHECK(shader == ClusterSlice(g, z));
    }
}

TEST_CASE("ClusterBlock: el bloque en cero apaga la grilla") {
    const ClusterBlock b {};
    CHECK(b.dims.w == 0u);
}
