#include <doctest/doctest.h>
#include <efengine/gameplay/CollisionMesh.h>

using efengine::gameplay::AppendSubmesh;
using efengine::gameplay::CollisionMesh;

namespace {
    const std::vector<glm::vec3> kTriangulo = {
        { 1.0f, 2.0f, 3.0f },
        { 4.0f, 5.0f, 6.0f },
        { 7.0f, 8.0f, 9.0f },
    };
    const std::vector<u32> kIndices = { 0u, 1u, 2u };
}

TEST_CASE("AppendSubmesh: una submalla aplana las posiciones a xyz y deja los indices igual") {
    CollisionMesh m;
    AppendSubmesh(m, kTriangulo, kIndices);

    REQUIRE(m.positions.size() == 9u);
    CHECK(m.positions[0] == doctest::Approx(1.0f));
    CHECK(m.positions[4] == doctest::Approx(5.0f));
    CHECK(m.positions[8] == doctest::Approx(9.0f));

    REQUIRE(m.indices.size() == 3u);
    CHECK(m.indices[0] == 0u);
    CHECK(m.indices[2] == 2u);
}

TEST_CASE("AppendSubmesh: la segunda submalla corre sus indices por los vertices de la primera") {
    // Este es EL caso del archivo. Sin el corrimiento la forma de colision sale
    // mal armada y no crashea nada: es un bug silencioso.
    CollisionMesh m;
    AppendSubmesh(m, kTriangulo, kIndices);
    AppendSubmesh(m, kTriangulo, kIndices);

    REQUIRE(m.positions.size() == 18u);
    REQUIRE(m.indices.size() == 6u);

    CHECK(m.indices[3] == 3u);
    CHECK(m.indices[4] == 4u);
    CHECK(m.indices[5] == 5u);
}

TEST_CASE("AppendSubmesh: una submalla incompleta no agrega nada") {
    CollisionMesh m;
    AppendSubmesh(m, {}, {});
    AppendSubmesh(m, kTriangulo, {});
    AppendSubmesh(m, {}, kIndices);

    CHECK(m.positions.empty());
    CHECK(m.indices.empty());
}
