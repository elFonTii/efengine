#include <doctest/doctest.h>
#include <efengine/renderer/Material.h>

#include <string>
#include <vector>

TEST_CASE("MakeUniformMaterialMap: una entrada por submesh, todas al mismo material") {
    using namespace efengine;
    renderer::Material m(nullptr);

    const renderer::MaterialMap map =
        renderer::MakeUniformMaterialMap({"rat", "eyes", "tail"}, &m);

    REQUIRE(map.size() == 3u);
    for (const auto& entrada : map) CHECK(entrada.second == &m);
    CHECK(map.count("eyes") == 1u);
}

TEST_CASE("MakeUniformMaterialMap: nombres repetidos colapsan en una entrada") {
    using namespace efengine;
    renderer::Material m(nullptr);

    // Dos submeshes pueden compartir materialName(): el mapa los unifica.
    const renderer::MaterialMap map = renderer::MakeUniformMaterialMap({"body", "body"}, &m);
    CHECK(map.size() == 1u);
}

TEST_CASE("MakeUniformMaterialMap: lista vacia -> mapa vacio") {
    using namespace efengine;
    renderer::Material m(nullptr);
    CHECK(renderer::MakeUniformMaterialMap({}, &m).empty());
}

TEST_CASE("MakeUniformMaterialMap: material nulo -> mapa vacio, no entradas nulas") {
    using namespace efengine;
    // Las entradas nulas son justo lo que hace que Renderer::Submit loguee un
    // warning por frame: mejor no crear ninguna.
    CHECK(renderer::MakeUniformMaterialMap({"body"}, nullptr).empty());
}
