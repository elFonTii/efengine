#include <doctest/doctest.h>
#include <efenet/Simulation.h>

using namespace efenet;

/*
    Step() es el corazon del PoC: la MISMA funcion la corren el servidor
    (authoritative) y la prediccion del cliente. Si deja de ser pura, la
    reconciliacion empieza a corregir todo el tiempo y el demo tiembla.

    Por eso el primer test no verifica movimiento sino PUREZA: dos corridas
    identicas desde el mismo estado tienen que dar exactamente lo mismo.
*/

namespace {
    InputCmd cmd(u8 buttons, f32 yaw = 0.0f, u32 seq = 1u) {
        InputCmd c;
        c.seq     = seq;
        c.yaw     = yaw;
        c.buttons = buttons;
        return c;
    }
}

TEST_CASE("efenet::Step es pura: la misma secuencia da el mismo resultado") {
    const InputCmd secuencia[] = {
        cmd(Button::Forward,             0.0f,  1u),
        cmd(Button::Forward | Button::Right, 0.5f,  2u),
        cmd(Button::Left,                1.2f,  3u),
        cmd(Button::Back,               -0.8f,  4u),
    };

    PlayerState a;
    PlayerState b;
    for (const InputCmd& c : secuencia) Step(a, c, kTickDt);
    for (const InputCmd& c : secuencia) Step(b, c, kTickDt);

    CHECK(a.position.x == doctest::Approx(b.position.x));
    CHECK(a.position.y == doctest::Approx(b.position.y));
    CHECK(a.position.z == doctest::Approx(b.position.z));
    CHECK(a.yaw        == doctest::Approx(b.yaw));
}

TEST_CASE("efenet::Step sin botones no mueve") {
    PlayerState p;
    p.position = glm::vec3(3.0f, 0.0f, -4.0f);

    Step(p, cmd(0u), kTickDt);

    CHECK(p.position.x == doctest::Approx(3.0f));
    CHECK(p.position.z == doctest::Approx(-4.0f));
}

TEST_CASE("efenet::Step copia el yaw del input al estado") {
    PlayerState p;
    Step(p, cmd(0u, 1.75f), kTickDt);
    CHECK(p.yaw == doctest::Approx(1.75f));
}

TEST_CASE("efenet::Step con yaw 0 y Forward avanza sobre -Z") {
    // Convencion de camara del motor: forward es -Z con yaw 0.
    PlayerState p;
    Step(p, cmd(Button::Forward, 0.0f), kTickDt);

    CHECK(p.position.z == doctest::Approx(-kMoveSpeed * kTickDt));
    CHECK(p.position.x == doctest::Approx(0.0f));
}

TEST_CASE("efenet::Step con yaw PI/2 y Forward avanza sobre otro eje") {
    PlayerState p;
    Step(p, cmd(Button::Forward, PI * 0.5f), kTickDt);

    // Rotado 90 grados: lo que era -Z ahora es un eje X puro.
    CHECK(p.position.z == doctest::Approx(0.0f).epsilon(0.001f));
    CHECK(std::abs(p.position.x) == doctest::Approx(kMoveSpeed * kTickDt));
}

TEST_CASE("efenet::Step en diagonal no es mas rapido que en recta") {
    // Sin normalizar, Forward+Right daria sqrt(2) veces la velocidad. Es el bug
    // clasico de movimiento, y en red se nota como desync entre los dos lados.
    PlayerState recta;
    PlayerState diagonal;

    Step(recta,    cmd(Button::Forward),                 kTickDt);
    Step(diagonal, cmd(Button::Forward | Button::Right), kTickDt);

    const f32 dRecta    = glm::length(recta.position);
    const f32 dDiagonal = glm::length(diagonal.position);

    CHECK(dDiagonal == doctest::Approx(dRecta));
}

TEST_CASE("efenet::Step clampea la posicion al area jugable") {
    PlayerState p;
    p.position = glm::vec3(kPlayAreaHalfSize - 0.01f, 0.0f, 0.0f);

    // Muchos ticks empujando hacia +X: no puede pasarse del borde.
    for (u32 i = 0u; i < 200u; ++i) Step(p, cmd(Button::Right, 0.0f), kTickDt);

    CHECK(p.position.x <= doctest::Approx(kPlayAreaHalfSize));
    CHECK(p.position.x >= doctest::Approx(-kPlayAreaHalfSize));
}

TEST_CASE("efenet::Step no toca la altura") {
    // No hay salto ni gravedad en el demo: Y queda fijo.
    PlayerState p;
    p.position = glm::vec3(0.0f, 1.0f, 0.0f);

    for (u32 i = 0u; i < 10u; ++i) Step(p, cmd(Button::Forward), kTickDt);

    CHECK(p.position.y == doctest::Approx(1.0f));
}
