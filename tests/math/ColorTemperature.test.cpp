#include <doctest/doctest.h>
#include <efengine/math/ColorTemperature.h>

#include <cmath>
#include <initializer_list>

using namespace efengine;

namespace {
    f32 Luminancia(const glm::vec3& c) { return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b; }
}

TEST_CASE("KelvinToLinearRgb: 6504 K (D65) sale casi neutro") {
    const glm::vec3 c = math::KelvinToLinearRgb(6504.0f);
    CHECK(c.r == doctest::Approx(1.0f).epsilon(0.05));
    CHECK(c.g == doctest::Approx(1.0f).epsilon(0.05));
    CHECK(c.b == doctest::Approx(1.0f).epsilon(0.05));
}

TEST_CASE("KelvinToLinearRgb: 2700 K es calido") {
    const glm::vec3 c = math::KelvinToLinearRgb(2700.0f);
    CHECK(c.r > c.g);
    CHECK(c.g > c.b);
}

TEST_CASE("KelvinToLinearRgb: 12000 K es frio") {
    const glm::vec3 c = math::KelvinToLinearRgb(12000.0f);
    CHECK(c.b > c.g);
    CHECK(c.g > c.r);
}

TEST_CASE("KelvinToLinearRgb: la luminancia es 1 en todo el rango") {
    for (f32 k : { 1667.0f, 2000.0f, 2222.0f, 2700.0f, 3500.0f, 4000.0f,
                   5000.0f, 6500.0f, 9000.0f, 15000.0f, 25000.0f }) {
        CAPTURE(k);
        CHECK(Luminancia(math::KelvinToLinearRgb(k)) == doctest::Approx(1.0f).epsilon(1e-4));
    }
}

TEST_CASE("KelvinToLinearRgb: fuera de rango se recorta") {
    const glm::vec3 bajo  = math::KelvinToLinearRgb(500.0f);
    const glm::vec3 min   = math::KelvinToLinearRgb(math::kMinKelvin);
    const glm::vec3 alto  = math::KelvinToLinearRgb(100000.0f);
    const glm::vec3 max   = math::KelvinToLinearRgb(math::kMaxKelvin);
    CHECK(bajo.r == doctest::Approx(min.r));
    CHECK(bajo.b == doctest::Approx(min.b));
    CHECK(alto.r == doctest::Approx(max.r));
    CHECK(alto.b == doctest::Approx(max.b));
}

TEST_CASE("KelvinToLinearRgb: sin saltos en los cambios de tramo") {
    for (f32 borde : { 2222.0f, 4000.0f }) {
        CAPTURE(borde);
        const glm::vec3 a = math::KelvinToLinearRgb(borde - 1.0f);
        const glm::vec3 b = math::KelvinToLinearRgb(borde + 1.0f);
        CHECK(std::abs(a.r - b.r) < 0.01f);
        CHECK(std::abs(a.g - b.g) < 0.01f);
        CHECK(std::abs(a.b - b.b) < 0.01f);
    }
}

TEST_CASE("KelvinToLinearRgb: nunca devuelve negativos") {
    for (f32 k = math::kMinKelvin; k <= math::kMaxKelvin; k += 250.0f) {
        const glm::vec3 c = math::KelvinToLinearRgb(k);
        CHECK(c.r >= 0.0f);
        CHECK(c.g >= 0.0f);
        CHECK(c.b >= 0.0f);
    }
}
