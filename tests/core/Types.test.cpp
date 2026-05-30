// tests/core/Types.test.cpp
// Test de ejemplo: valida las constantes y aliases de core/Types.h.
// No necesita contexto OpenGL, así prueba el pipeline de punta a punta.
#include <doctest/doctest.h>
#include <efengine/core/Types.h>

TEST_CASE("Types: relaciones entre constantes angulares") {
    CHECK(TAU == doctest::Approx(2.0f * PI));
    // Convierte 90° → rad → de vuelta a grados: ejercita la cadena de
    // conversión con un valor concreto (no se pliega a una tautología).
    CHECK(RAD_TO_DEG * (90.0f * DEG_TO_RAD) == doctest::Approx(90.0f));
    CHECK(EPSILON > 0.0f);
}

TEST_CASE("Types: tamaños de los aliases enteros") {
    CHECK(sizeof(u8)  == 1);
    CHECK(sizeof(u16) == 2);
    CHECK(sizeof(u32) == 4);
    CHECK(sizeof(u64) == 8);
}
