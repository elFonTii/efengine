#include <doctest/doctest.h>
#include <efengine/renderer/LightMath.h>

#include <glm/glm.hpp>
#include <cmath>
#include <initializer_list>
#include <limits>

using namespace efengine;
using namespace efengine::renderer;

TEST_CASE("LightFalloff: cerca de la fuente es inverse-square") {
    CHECK(LightFalloff(1.0f, 100.0f, 0.0f) == doctest::Approx(1.0f).epsilon(1e-3));
    CHECK(LightFalloff(2.0f, 100.0f, 0.0f) == doctest::Approx(0.25f).epsilon(1e-3));
}

TEST_CASE("LightFalloff: vale cero exacto en el rango y afuera") {
    CHECK(LightFalloff(10.0f, 10.0f, 0.0f) == 0.0f);
    CHECK(LightFalloff(12.0f, 10.0f, 0.0f) == 0.0f);
}

TEST_CASE("LightFalloff: es monotona decreciente") {
    f32 anterior = std::numeric_limits<f32>::max();
    for (f32 d = 0.1f; d < 10.0f; d += 0.1f) {
        const f32 v = LightFalloff(d, 10.0f, 0.0f);
        CHECK(v <= anterior);
        anterior = v;
    }
}

TEST_CASE("LightFalloff: pegado a la fuente es finita") {
    CHECK(LightFalloff(0.0f, 10.0f, 0.0f) == doctest::Approx(10000.0f));
    CHECK(LightFalloff(0.0f, 10.0f, 0.5f) == doctest::Approx(4.0f));
}

TEST_CASE("LightFalloff: rango cero no ilumina") {
    CHECK(LightFalloff(1.0f, 0.0f, 0.0f) == 0.0f);
}

TEST_CASE("SpotParams: 1 adentro del cono interior, 0 afuera del exterior") {
    const f32 ci = std::cos(glm::radians(30.0f));
    const f32 co = std::cos(glm::radians(45.0f));
    const SpotAngleParams p = SpotParams(ci, co);
    CHECK(SpotAngular(p, 1.0f) == doctest::Approx(1.0f));
    CHECK(SpotAngular(p, ci)   == doctest::Approx(1.0f));
    CHECK(SpotAngular(p, co)   == doctest::Approx(0.0f));
    CHECK(SpotAngular(p, std::cos(glm::radians(60.0f))) == 0.0f);
    CHECK(SpotAngular(p, (ci + co) * 0.5f) == doctest::Approx(0.25f));
}

TEST_CASE("SpotParams: inner == outer no divide por cero") {
    const SpotAngleParams p = SpotParams(0.8f, 0.8f);
    CHECK(std::isfinite(p.scale));
    CHECK(SpotAngular(p, 0.81f) == 1.0f);
    CHECK(SpotAngular(p, 0.79f) == 0.0f);
}

TEST_CASE("SpotBoundingSphere: contiene el apice, la punta y el borde del cono") {
    const glm::vec3 p(1.0f, 2.0f, 3.0f);
    const glm::vec3 d(0.0f, -1.0f, 0.0f);
    const f32 R = 8.0f;
    for (f32 deg : { 10.0f, 30.0f, 45.0f, 60.0f, 80.0f }) {
        CAPTURE(deg);
        const f32 c = std::cos(glm::radians(deg));
        const f32 s = std::sin(glm::radians(deg));
        const BoundingSphere b = SpotBoundingSphere(p, d, R, c);
        auto dentro = [&](const glm::vec3& q) {
            return glm::length(q - b.center) <= b.radius * 1.0001f + 1e-4f;
        };
        CHECK(dentro(p));
        CHECK(dentro(p + d * R));
        CHECK(dentro(p + R * glm::vec3(s, -c, 0.0f)));
        if (deg <= 45.0f) CHECK(b.radius <= R);
    }
}

TEST_CASE("SphereIntersectsAabb: adentro, tocando y afuera") {
    const glm::vec3 mn(0.0f), mx(1.0f);
    CHECK(SphereIntersectsAabb({ glm::vec3(0.5f), 0.1f }, mn, mx));
    CHECK(SphereIntersectsAabb({ glm::vec3(2.0f, 0.5f, 0.5f), 1.0f }, mn, mx));
    CHECK_FALSE(SphereIntersectsAabb({ glm::vec3(2.0f, 0.5f, 0.5f), 0.99f }, mn, mx));
    CHECK_FALSE(SphereIntersectsAabb({ glm::vec3(2.0f), 1.7f }, mn, mx));
    CHECK(SphereIntersectsAabb({ glm::vec3(2.0f), 1.74f }, mn, mx));
}

TEST_CASE("ConeIntersectsSphere: eje, costado, detras, lejos y punta") {
    const glm::vec3 apex(0.0f), dir(0.0f, 0.0f, -1.0f);
    const f32 R = 10.0f;
    const f32 c = std::cos(glm::radians(30.0f));
    CHECK(ConeIntersectsSphere(apex, dir, R, c, { glm::vec3(0.0f, 0.0f, -5.0f), 0.5f }));
    CHECK_FALSE(ConeIntersectsSphere(apex, dir, R, c, { glm::vec3(5.0f, 0.0f, -1.0f), 0.5f }));
    CHECK_FALSE(ConeIntersectsSphere(apex, dir, R, c, { glm::vec3(0.0f, 0.0f, 3.0f), 0.5f }));
    CHECK_FALSE(ConeIntersectsSphere(apex, dir, R, c, { glm::vec3(0.0f, 0.0f, -12.0f), 1.0f }));
    CHECK(ConeIntersectsSphere(apex, dir, R, c, { glm::vec3(0.0f, 0.0f, -10.5f), 1.0f }));
}

TEST_CASE("LocalLightTouchesAabb: un spot no toca la caja que su esfera de rango si") {
    // Caja al costado del spot: adentro de la esfera de rango, afuera del cono.
    const glm::vec3 mn(4.0f, -0.5f, -1.5f), mx(5.0f, 0.5f, -0.5f);
    const glm::vec3 pos(0.0f), dir(0.0f, 0.0f, -1.0f);
    const f32 c = std::cos(glm::radians(20.0f));
    CHECK(LocalLightTouchesAabb(LightType::Point, pos, dir, 10.0f, c, mn, mx));
    CHECK_FALSE(LocalLightTouchesAabb(LightType::Spot, pos, dir, 10.0f, c, mn, mx));
}
