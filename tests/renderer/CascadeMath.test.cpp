// Los cortes son la unica parte del CSM que decide como se reparte la resolucion
// entre cerca y lejos. Un reparto mal hecho no rompe nada visible de golpe: deja
// la cascada 0 desperdiciada y la ultima pastosa, que es dificil de diagnosticar
// mirando la pantalla.
#include <doctest/doctest.h>
#include <efengine/renderer/ShadowMath.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

using efengine::renderer::ComputeCascadeSplits;
using efengine::renderer::kMaxCascades;

TEST_CASE("ComputeCascadeSplits: el ultimo corte es exactamente shadowDistance") {
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(0.1f, 200.0f, 4u, 0.75f, fars);
    CHECK(fars[3] == doctest::Approx(200.0f));
}

TEST_CASE("ComputeCascadeSplits: los cortes son estrictamente crecientes") {
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(0.1f, 200.0f, 4u, 0.75f, fars);
    CHECK(fars[0] > 0.1f);
    CHECK(fars[1] > fars[0]);
    CHECK(fars[2] > fars[1]);
    CHECK(fars[3] > fars[2]);
}

TEST_CASE("ComputeCascadeSplits: lambda 0 da el reparto uniforme") {
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(0.0f, 200.0f, 4u, 0.0f, fars);
    CHECK(fars[0] == doctest::Approx(50.0f));
    CHECK(fars[1] == doctest::Approx(100.0f));
    CHECK(fars[2] == doctest::Approx(150.0f));
    CHECK(fars[3] == doctest::Approx(200.0f));
}

TEST_CASE("ComputeCascadeSplits: lambda 1 da el reparto logaritmico") {
    // log: near * (far/near)^(i/N). Con near=1, far=16, N=4 -> 2, 4, 8, 16.
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(1.0f, 16.0f, 4u, 1.0f, fars);
    CHECK(fars[0] == doctest::Approx(2.0f));
    CHECK(fars[1] == doctest::Approx(4.0f));
    CHECK(fars[2] == doctest::Approx(8.0f));
    CHECK(fars[3] == doctest::Approx(16.0f));
}

TEST_CASE("ComputeCascadeSplits: una sola cascada cubre todo el rango") {
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(0.1f, 200.0f, 1u, 0.75f, fars);
    CHECK(fars[0] == doctest::Approx(200.0f));
}

TEST_CASE("ComputeCascadeSplits: lambda intermedio queda entre los dos extremos") {
    f32 uniforme[kMaxCascades] = {};
    f32 logaritmico[kMaxCascades] = {};
    f32 mezcla[kMaxCascades] = {};
    ComputeCascadeSplits(1.0f, 16.0f, 4u, 0.0f, uniforme);
    ComputeCascadeSplits(1.0f, 16.0f, 4u, 1.0f, logaritmico);
    ComputeCascadeSplits(1.0f, 16.0f, 4u, 0.5f, mezcla);
    for (u32 i = 0; i < 3u; ++i) {
        CHECK(mezcla[i] <= uniforme[i]);
        CHECK(mezcla[i] >= logaritmico[i]);
    }
}

TEST_CASE("ComputeCascadeSplits: un near mayor que shadowDistance no produce NaN") {
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(300.0f, 200.0f, 4u, 0.75f, fars);
    for (u32 i = 0; i < 4u; ++i) {
        CHECK(std::isfinite(fars[i]));
    }
}
