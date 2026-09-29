#include <doctest/doctest.h>
#include <efengine/renderer/DdgiGridFit.h>

using namespace efengine;
using namespace efengine::renderer;

namespace {
    AABB Caja(glm::vec3 mn, glm::vec3 mx) { AABB b; b.min = mn; b.max = mx; return b; }
}

TEST_CASE("FitDdgiGridToBounds: margen del 10% hacia adentro y counts conservados") {
    DdgiGrid g;
    g.counts = glm::ivec3(10, 5, 9);
    REQUIRE(FitDdgiGridToBounds(g, Caja({-10.0f, 0.0f, -20.0f}, {10.0f, 4.0f, 20.0f})));

    CHECK(g.counts == glm::ivec3(10, 5, 9));
    CHECK(g.origin.x  == doctest::Approx(-9.0f));
    CHECK(g.origin.y  == doctest::Approx(0.2f));
    CHECK(g.origin.z  == doctest::Approx(-18.0f));
    CHECK(g.spacing.x == doctest::Approx(2.0f));
    CHECK(g.spacing.y == doctest::Approx(0.9f));
    CHECK(g.spacing.z == doctest::Approx(4.5f));
}

TEST_CASE("FitDdgiGridToBounds: un eje con un solo probe usa spacing 1") {
    DdgiGrid g;
    g.counts = glm::ivec3(4, 1, 4);
    REQUIRE(FitDdgiGridToBounds(g, Caja({0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f})));
    CHECK(g.spacing.y == doctest::Approx(1.0f));
}

TEST_CASE("FitDdgiGridToBounds: una caja plana no deja spacing en cero") {
    DdgiGrid g;
    g.counts = glm::ivec3(4, 4, 4);
    REQUIRE(FitDdgiGridToBounds(g, Caja({0.0f, 2.0f, 0.0f}, {10.0f, 2.0f, 10.0f})));
    CHECK(g.spacing.y >= 0.1f);
}

TEST_CASE("FitDdgiGridToBounds: bounds invalidos no tocan la grilla") {
    DdgiGrid g;
    g.origin  = glm::vec3(1.0f, 2.0f, 3.0f);
    g.spacing = glm::vec3(4.0f);
    CHECK_FALSE(FitDdgiGridToBounds(g, AABB::Empty()));
    CHECK(g.origin  == glm::vec3(1.0f, 2.0f, 3.0f));
    CHECK(g.spacing == glm::vec3(4.0f));
}
