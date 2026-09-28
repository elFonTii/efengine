#include <doctest/doctest.h>
#include <efengine/renderer/LightPacking.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstddef>
#include <vector>

using namespace efengine;
using namespace efengine::renderer;

namespace {
    // Camara en el origen mirando a -Z con far enorme: todo lo que esta en z < 0 se ve.
    Frustum FrustumAmplio() {
        return ExtractFrustum(glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 1.0e6f));
    }

    Light Punto(const glm::vec3& pos, f32 range) {
        Light l;
        l.type     = LightType::Point;
        l.position = pos;
        l.range    = range;
        l.color    = glm::vec3(1.0f);
        return l;
    }

    Light Direccional(f32 marca, bool primario) {
        Light l;
        l.type       = LightType::Directional;
        l.direction  = glm::vec3(0.0f, -1.0f, 0.0f);
        l.color      = glm::vec3(marca);
        l.primarySun = primario;
        return l;
    }
}

TEST_CASE("Layout std430: GpuLight mide 80 y sus vec4 caen donde el shader los lee") {
    CHECK(sizeof(GpuLight) == 80u);
    CHECK(offsetof(GpuLight, positionRange) ==  0u);
    CHECK(offsetof(GpuLight, colorRadius)   == 16u);
    CHECK(offsetof(GpuLight, directionType) == 32u);
    CHECK(offsetof(GpuLight, spotParams)    == 48u);
    CHECK(offsetof(GpuLight, reserved)      == 64u);
}

TEST_CASE("Layout std140: LightsBlock mide 144") {
    CHECK(sizeof(LightsBlock) == 144u);
    CHECK(offsetof(LightsBlock, dirDirection) ==   0u);
    CHECK(offsetof(LightsBlock, dirColor)     ==  64u);
    CHECK(offsetof(LightsBlock, counts)       == 128u);
}

TEST_CASE("PackGpuLight: posicion, rango, color, radio y tipo") {
    Light l = Punto(glm::vec3(1.0f, 2.0f, 3.0f), 7.0f);
    l.color        = glm::vec3(4.0f, 5.0f, 6.0f);
    l.sourceRadius = 0.25f;
    const GpuLight g = PackGpuLight(l);
    CHECK(g.positionRange == glm::vec4(1.0f, 2.0f, 3.0f, 7.0f));
    CHECK(g.colorRadius   == glm::vec4(4.0f, 5.0f, 6.0f, 0.25f));
    CHECK(g.directionType.w == 0.0f);
}

TEST_CASE("PackGpuLight: el spot viaja con scale/offset y cos/sin del cono exterior") {
    Light l;
    l.type     = LightType::Spot;
    l.cosInner = std::cos(glm::radians(30.0f));
    l.cosOuter = std::cos(glm::radians(45.0f));
    const GpuLight g = PackGpuLight(l);
    const SpotAngleParams p = SpotParams(l.cosInner, l.cosOuter);
    CHECK(g.directionType.w == 1.0f);
    CHECK(g.spotParams.x == doctest::Approx(p.scale));
    CHECK(g.spotParams.y == doctest::Approx(p.offset));
    CHECK(g.spotParams.z == doctest::Approx(l.cosOuter));
    CHECK(g.spotParams.w == doctest::Approx(std::sin(glm::radians(45.0f))));
}

TEST_CASE("PackGpuLight: reserved deja shadowIndex en -1 y el flag de sombra") {
    Light l = Punto(glm::vec3(0.0f), 1.0f);
    l.castShadows = true;
    const GpuLight g = PackGpuLight(l);
    CHECK(g.reserved.x == -1.0f);
    CHECK(g.reserved.y == 1.0f);
}

TEST_CASE("PackLights: sin luces todo queda en cero") {
    PackedLights p;
    PackLights({}, FrustumAmplio(), p);
    CHECK(p.block.counts == glm::uvec4(0u));
    CHECK(p.locals.empty());
    CHECK(p.visible.empty());
}

TEST_CASE("PackLights: el sol primario va al indice 0 aunque llegue ultimo") {
    std::vector<Light> luces;
    for (int i = 1; i <= 4; ++i) luces.push_back(Direccional(static_cast<f32>(i), false));
    luces.push_back(Direccional(9.0f, true));

    PackedLights p;
    PackLights(luces, FrustumAmplio(), p);
    CHECK(p.block.counts.y == kMaxDirectionalLights);
    CHECK(p.totalDirectional == 5u);
    CHECK(p.block.dirColor[0].x == doctest::Approx(9.0f));
    CHECK(p.block.dirColor[0].w == 1.0f);
    for (u32 i = 1u; i < kMaxDirectionalLights; ++i) CHECK(p.block.dirColor[i].w == 0.0f);
    CHECK(p.locals.empty());
}

TEST_CASE("PackLights: las locales fuera del frustum se suben pero no son visibles") {
    const std::vector<Light> luces { Punto(glm::vec3(0.0f, 0.0f, -10.0f), 2.0f),
                                     Punto(glm::vec3(0.0f, 0.0f,  50.0f), 2.0f) };
    PackedLights p;
    PackLights(luces, FrustumAmplio(), p);
    CHECK(p.locals.size() == 2u);
    REQUIRE(p.visible.size() == 1u);
    CHECK(p.visible[0] == 0u);
    CHECK(p.block.counts.x == 2u);
    CHECK(p.block.counts.z == 1u);
}

TEST_CASE("PackLights: mas de kMaxLocalLights se recortan y se cuentan") {
    std::vector<Light> luces(kMaxLocalLights + 3u, Punto(glm::vec3(0.0f, 0.0f, -10.0f), 1.0f));
    PackedLights p;
    PackLights(luces, FrustumAmplio(), p);
    CHECK(p.locals.size() == kMaxLocalLights);
    CHECK(p.totalLocals == kMaxLocalLights + 3u);
    CHECK(p.visible.size() == kMaxLocalLights);
}

TEST_CASE("PackLights: reusar el mismo PackedLights no arrastra el frame anterior") {
    PackedLights p;
    PackLights(std::vector<Light>(3u, Punto(glm::vec3(0.0f, 0.0f, -5.0f), 1.0f)), FrustumAmplio(), p);
    PackLights(std::vector<Light>(1u, Punto(glm::vec3(0.0f, 0.0f, -5.0f), 1.0f)), FrustumAmplio(), p);
    CHECK(p.locals.size() == 1u);
    CHECK(p.visible.size() == 1u);
    CHECK(p.block.counts.x == 1u);
}

TEST_CASE("LocalLightBounds: point es su esfera de rango, spot la del cono") {
    const Light pt = Punto(glm::vec3(1.0f, 2.0f, 3.0f), 5.0f);
    const BoundingSphere a = LocalLightBounds(pt);
    CHECK(a.center == pt.position);
    CHECK(a.radius == doctest::Approx(5.0f));

    Light sp = pt;
    sp.type      = LightType::Spot;
    sp.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    sp.cosOuter  = std::cos(glm::radians(20.0f));
    const BoundingSphere b = LocalLightBounds(sp);
    CHECK(b.radius < 5.0f);
    CHECK(b.center.y < pt.position.y);
}
