// El clamp de anisotropia es la unica parte sin GL del filtrado anisotropico, y
// la unica que puede romper en una maquina distinta: GL da error si se le pide
// mas que GL_MAX_TEXTURE_MAX_ANISOTROPY, que no es 16 en todo hardware.
#include <doctest/doctest.h>
#include <efecom/RHI.h>

using efecom::ClampAnisotropy;

TEST_CASE("ClampAnisotropy: el device da lo que se pide") {
    CHECK(ClampAnisotropy(16.0f, 16.0f) == doctest::Approx(16.0f));
    CHECK(ClampAnisotropy(4.0f, 16.0f) == doctest::Approx(4.0f));
}

TEST_CASE("ClampAnisotropy: el device da menos de lo que se pide") {
    CHECK(ClampAnisotropy(16.0f, 8.0f) == doctest::Approx(8.0f));
    CHECK(ClampAnisotropy(16.0f, 1.0f) == doctest::Approx(1.0f));
}

TEST_CASE("ClampAnisotropy: el piso es 1.0, nunca menos") {
    CHECK(ClampAnisotropy(0.0f, 16.0f) == doctest::Approx(1.0f));
    CHECK(ClampAnisotropy(-4.0f, 16.0f) == doctest::Approx(1.0f));
}

TEST_CASE("ClampAnisotropy: un deviceMax invalido no baja del piso") {
    CHECK(ClampAnisotropy(16.0f, 0.0f) == doctest::Approx(1.0f));
}
