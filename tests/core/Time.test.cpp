
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

/*
    Paso fijo. Mismo criterio que los casos de arriba: timestamps sintéticos vía
    Advance(f64), sin tocar el reloj real.

    Varios casos suben maxDelta antes de probar: el clamp de 0.1 s existe para el
    dt variable y taparía justo el caso que se quiere ver.
*/

TEST_CASE("Time: el primer Advance no produce pasos fijos") {
    core::Time t;
    t.Advance(100.0);

    CHECK(t.FixedSteps() == 0);
    CHECK(t.Alpha() == doctest::Approx(0.0f));
    CHECK_FALSE(t.FixedStepsSaturated());
}

TEST_CASE("Time: un frame mas largo que el paso fijo produce un paso y deja resto") {
    core::Time t;
    t.SetFixedDelta(0.02f);
    t.Advance(100.0);
    t.Advance(100.025);   // 0.025 = un paso de 0.02 + 0.005 de resto

    CHECK(t.FixedSteps() == 1);
    CHECK(t.Alpha() == doctest::Approx(0.25f));   // 0.005 / 0.02
}

TEST_CASE("Time: un frame mas corto que el paso fijo no produce pasos pero acumula") {
    core::Time t;
    t.SetFixedDelta(0.02f);
    t.Advance(100.0);

    t.Advance(100.011);          // 0.011: todavia no alcanza
    CHECK(t.FixedSteps() == 0);
    CHECK(t.Alpha() == doctest::Approx(0.55f));

    t.Advance(100.022);          // 0.011 + 0.011 = 0.022: ahora si
    CHECK(t.FixedSteps() == 1);
    CHECK(t.Alpha() == doctest::Approx(0.1f));   // 0.002 / 0.02
}

TEST_CASE("Time: un frame que vale varios pasos los entrega todos") {
    core::Time t;
    t.SetMaxDelta(1.0f);
    t.SetFixedDelta(0.02f);
    t.Advance(100.0);
    t.Advance(100.065);          // 3 pasos de 0.02 + 0.005 de resto

    CHECK(t.FixedSteps() == 3);
    CHECK(t.Alpha() == doctest::Approx(0.25f));
}

TEST_CASE("Time: dos Advance con el mismo timestamp no producen pasos") {
    core::Time t;
    t.SetFixedDelta(0.02f);
    t.Advance(100.0);
    t.Advance(100.03);
    const f32 alphaAntes = t.Alpha();

    t.Advance(100.03);           // dt = 0: el frame no avanzo nada

    CHECK(t.FixedSteps() == 0);
    CHECK(t.Alpha() == doctest::Approx(alphaAntes));   // el acumulador no se movio
    CHECK_FALSE(t.FixedStepsSaturated());
}

TEST_CASE("Time: el tope de pasos descarta el sobrante en vez de acumularlo") {
    core::Time t;
    t.SetMaxDelta(1.0f);
    t.SetFixedDelta(0.02f);
    t.SetMaxFixedSteps(5);
    t.Advance(100.0);
    t.Advance(101.0);            // 1 segundo = 50 pasos pedidos, tope 5

    CHECK(t.FixedSteps() == 5);
    CHECK(t.FixedStepsSaturated());
    CHECK(t.Alpha() == doctest::Approx(0.0f));

    // El frame siguiente arranca limpio. Si el sobrante se acumulara, cada frame
    // lento pediria mas pasos que el anterior y la simulacion nunca se pondria al
    // dia: eso es la espiral de la muerte, y este CHECK es lo que la impide.
    t.Advance(101.025);
    CHECK(t.FixedSteps() == 1);
    CHECK_FALSE(t.FixedStepsSaturated());
}

TEST_CASE("Time: Alpha nunca llega a 1") {
    core::Time t;
    t.SetMaxDelta(1.0f);
    t.SetFixedDelta(0.02f);
    t.Advance(100.0);

    // Barrido de frames de largos distintos: en ninguno alpha puede tocar 1.0,
    // porque en cuanto el acumulador alcanza un paso, ese paso se consume.
    f64 now = 100.0;
    for (int i = 1; i <= 40; ++i) {
        now += 0.001 * i;
        t.Advance(now);
        CHECK(t.Alpha() >= 0.0f);
        CHECK(t.Alpha() < 1.0f);
    }
}

TEST_CASE("Time: el default es 60 Hz") {
    core::Time t;
    CHECK(t.FixedDelta() == doctest::Approx(1.0f / 60.0f));
}
