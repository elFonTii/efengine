
#include <doctest/doctest.h>
#include <efengine/core/Time.h>

using namespace efengine;

/*
    Todos los casos usan Advance(f64) en vez de Tick(): le pasan timestamps
    sintéticos, así la matemática se verifica sin leer el reloj real y sin
    contexto GL. Los timestamps arrancan en 100.0 a propósito, para que no se
    confunda "el tiempo desde el primer Advance" con "el valor del timestamp".
*/

TEST_CASE("Time::Advance el primer tick no inventa tiempo") {
    core::Time t;
    t.Advance(100.0);

    CHECK(t.DeltaTime() == doctest::Approx(0.0f));
    CHECK(t.Elapsed()   == doctest::Approx(0.0));
}

TEST_CASE("Time::DeltaTime reporta los segundos entre dos Advance") {
    core::Time t;
    t.Advance(100.0);
    t.Advance(100.016);

    CHECK(t.DeltaTime() == doctest::Approx(0.016f));
}

TEST_CASE("Time::DeltaTime se clampea a 0.1 por default") {
    core::Time t;
    t.Advance(100.0);
    t.Advance(105.0); // un salto de 5 segundos: breakpoint, carga de assets, etc.

    CHECK(t.DeltaTime() == doctest::Approx(0.1f));
}

TEST_CASE("Time::SetMaxDelta mueve el techo del clamp") {
    core::Time t;
    t.SetMaxDelta(0.25f);
    t.Advance(100.0);
    t.Advance(105.0);

    CHECK(t.DeltaTime() == doctest::Approx(0.25f));
}

TEST_CASE("Time::Elapsed no se clampea aunque DeltaTime sí") {
    core::Time t;
    t.Advance(100.0);
    t.Advance(105.0);

    // La asimetría es deliberada: el clamp existe para que un frame largo no
    // teletransporte la simulación, pero Elapsed es tiempo de pared y no puede
    // desincronizarse en silencio del reloj. Si alguien "simplifica" clampeando
    // los dos, este CHECK se pone rojo.
    CHECK(t.Elapsed()   == doctest::Approx(5.0));
    CHECK(t.DeltaTime() == doctest::Approx(0.1f));
}

TEST_CASE("Time::Elapsed acumula a través de varios ticks") {
    core::Time t;
    t.Advance(100.0);
    t.Advance(100.016);
    t.Advance(100.032);

    CHECK(t.Elapsed()   == doctest::Approx(0.032));
    CHECK(t.DeltaTime() == doctest::Approx(0.016f));
}
