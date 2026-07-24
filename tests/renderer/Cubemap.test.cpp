// tests/renderer/Cubemap.test.cpp
// Valida mipLevels: la cantidad de niveles de mip de una textura cuadrada.
// Lógica pura (floor(log2(size))+1), sin contexto OpenGL.
#include <doctest/doctest.h>
#include <efengine/renderer/Cubemap.h>

using efengine::renderer::mipLevels;

TEST_CASE("mipLevels: potencias de dos") {
    CHECK(mipLevels(512u) == 10u);  // 512,256,...,1  -> 10 niveles
    CHECK(mipLevels(128u) == 8u);   // 128,...,1       -> 8
    CHECK(mipLevels(32u)  == 6u);   // 32,...,1        -> 6
    CHECK(mipLevels(2u)   == 2u);
}

TEST_CASE("mipLevels: casos borde y no-potencias-de-dos") {
    CHECK(mipLevels(1u) == 1u);
    CHECK(mipLevels(0u) == 1u);     // por convencion, al menos 1 nivel
    CHECK(mipLevels(500u) == 9u);   // floor(log2(500))=8 -> 9
}
