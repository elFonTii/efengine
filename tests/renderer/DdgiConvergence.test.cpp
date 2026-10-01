// La medicion de convergencia es logica pura sobre muestras que llegan de la GPU con
// atraso: se testea sin contexto GL.
#include <doctest/doctest.h>
#include <efengine/renderer/DdgiConvergence.h>
#include <efengine/renderer/DdgiSettings.h>

using namespace efengine;
using namespace efengine::renderer;

TEST_CASE("MeanDelta: punto fijo a float, y negativo sin texels") {
    DdgiGpuStats s {};
    CHECK(MeanDelta(s) < 0.0f);
    s.deltaSum   = 12000u;
    s.deltaCount = 4u;
    CHECK(MeanDelta(s) == doctest::Approx(0.3f));
}

TEST_CASE("DdgiConvergence: arranca sin converger") {
    const DdgiConvergence c;
    CHECK_FALSE(c.converged());
}

TEST_CASE("DdgiConvergence: ignora muestras producidas antes del reset") {
    DdgiConvergence c;
    c.Reset(100u, 10.0);
    c.Observe(99u, 10.05, 1e-5f, 6e-4f);    // frame de antes del reset, leida tarde
    c.Observe(100u, 10.06, 1e-5f, 6e-4f);   // el mismo frame del reset tampoco cuenta
    CHECK_FALSE(c.converged());
}

TEST_CASE("DdgiConvergence: converge en la primera muestra bajo epsilon y mide desde el reset") {
    DdgiConvergence c;
    c.Reset(100u, 10.0);
    c.Observe(101u, 10.1, 0.8f, 6e-4f);
    c.Observe(150u, 10.9, 7e-4f, 6e-4f);
    CHECK_FALSE(c.converged());
    c.Observe(190u, 11.5, 5e-4f, 6e-4f);
    REQUIRE(c.converged());
    CHECK(c.seconds() == doctest::Approx(1.5));
}

TEST_CASE("DdgiConvergence: una vez convergido, las muestras siguientes no lo mueven") {
    DdgiConvergence c;
    c.Reset(0u, 0.0);
    c.Observe(10u, 1.0, 1e-4f, 6e-4f);
    c.Observe(20u, 2.0, 1e-5f, 6e-4f);
    CHECK(c.seconds() == doctest::Approx(1.0));
}

TEST_CASE("DdgiConvergence: muestras sin texels (delta negativa) no cuentan") {
    DdgiConvergence c;
    c.Reset(0u, 0.0);
    c.Observe(5u, 0.5, -1.0f, 6e-4f);
    CHECK_FALSE(c.converged());
}

TEST_CASE("DdgiConvergence: un reset nuevo vuelve a medir") {
    DdgiConvergence c;
    c.Reset(0u, 0.0);
    c.Observe(10u, 1.0, 1e-4f, 6e-4f);
    c.Reset(50u, 5.0);
    CHECK_FALSE(c.converged());
    c.Observe(60u, 5.25, 1e-4f, 6e-4f);
    CHECK(c.seconds() == doctest::Approx(0.25));
}

TEST_CASE("DdgiGpuStats espeja el bloque std430 de 32 bytes") {
    CHECK(sizeof(DdgiGpuStats) == 32u);
    CHECK(kDdgiStatsBinding == 7u);
}

TEST_CASE("DdgiSettings: epsilon de convergencia medido en la simulacion") {
    CHECK(DdgiSettings{}.convergenceEpsilon == doctest::Approx(6.0e-4f));
}
