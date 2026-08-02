#include <doctest/doctest.h>
#include <efenet/SnapshotBuffer.h>

using namespace efenet;

/*
    Un avatar remoto se dibuja kInterpDelay en el pasado: se toman los dos
    snapshots que rodean ese instante y se interpola. El precio es ver a los
    demas 100 ms atrasados; el premio es que se mueven suave aunque los
    snapshots lleguen a 20 Hz y con perdida.
*/

namespace {
    PlayerState en(f32 x, f32 yaw = 0.0f) {
        PlayerState p;
        p.id       = 1u;
        p.position = glm::vec3(x, 0.0f, 0.0f);
        p.yaw      = yaw;
        return p;
    }
}

TEST_CASE("efenet::LerpAngle interpola por el camino corto cruzando PI") {
    // De 170 a -170 grados hay 20 grados por el camino corto, no 340. Sin esto,
    // un avatar que gira cruzando el borde pega una vuelta entera al reves.
    const f32 a = 170.0f * DEG_TO_RAD;
    const f32 b = -170.0f * DEG_TO_RAD;

    const f32 medio = LerpAngle(a, b, 0.5f);

    // El punto medio por el camino corto es 180 (o -180: son el mismo angulo).
    CHECK(std::abs(std::abs(medio) - PI) < 0.01f);
}

TEST_CASE("efenet::LerpAngle en t=0 y t=1 devuelve los extremos") {
    const f32 a = 0.5f;
    const f32 b = 2.0f;

    CHECK(LerpAngle(a, b, 0.0f) == doctest::Approx(a));
    CHECK(LerpAngle(a, b, 1.0f) == doctest::Approx(b).epsilon(0.001f));
}

TEST_CASE("efenet::LerpAngle sin cruce de borde interpola normal") {
    CHECK(LerpAngle(0.0f, 1.0f, 0.5f) == doctest::Approx(0.5f));
}

TEST_CASE("efenet::SnapshotBuffer vacio no puede muestrear") {
    SnapshotBuffer buf;
    PlayerState out;
    CHECK_FALSE(buf.Sample(0.0f, true, out));
    CHECK(buf.Count() == 0u);
}

TEST_CASE("efenet::SnapshotBuffer con un solo snapshot devuelve ese") {
    SnapshotBuffer buf;
    buf.Push(1.0f, en(5.0f));

    PlayerState out;
    REQUIRE(buf.Sample(1.0f, true, out));
    CHECK(out.position.x == doctest::Approx(5.0f));
}

TEST_CASE("efenet::SnapshotBuffer muestrear en el tiempo exacto da el valor exacto") {
    SnapshotBuffer buf;
    buf.Push(1.0f, en(10.0f));
    buf.Push(2.0f, en(20.0f));

    PlayerState out;
    REQUIRE(buf.Sample(2.0f, true, out));
    CHECK(out.position.x == doctest::Approx(20.0f));
}

TEST_CASE("efenet::SnapshotBuffer interpola en el punto medio") {
    SnapshotBuffer buf;
    buf.Push(1.0f, en(10.0f));
    buf.Push(2.0f, en(20.0f));

    PlayerState out;
    REQUIRE(buf.Sample(1.5f, true, out));
    CHECK(out.position.x == doctest::Approx(15.0f));
}

TEST_CASE("efenet::SnapshotBuffer interpola en un cuarto del intervalo") {
    SnapshotBuffer buf;
    buf.Push(1.0f, en(0.0f));
    buf.Push(2.0f, en(100.0f));

    PlayerState out;
    REQUIRE(buf.Sample(1.25f, true, out));
    CHECK(out.position.x == doctest::Approx(25.0f));
}

TEST_CASE("efenet::SnapshotBuffer clampea antes del snapshot mas viejo") {
    // Recien conectado: el renderTime todavia cae antes de lo que tenemos.
    // Mejor congelar en el mas viejo que extrapolar hacia atras.
    SnapshotBuffer buf;
    buf.Push(5.0f, en(50.0f));
    buf.Push(6.0f, en(60.0f));

    PlayerState out;
    REQUIRE(buf.Sample(1.0f, true, out));
    CHECK(out.position.x == doctest::Approx(50.0f));
}

TEST_CASE("efenet::SnapshotBuffer extrapola con la velocidad de los dos ultimos") {
    // Por eso el snapshot no necesita mandar velocidad: se deriva.
    SnapshotBuffer buf;
    buf.Push(1.0f, en(10.0f));
    buf.Push(2.0f, en(20.0f));   // 10 unidades por segundo

    PlayerState out;
    REQUIRE(buf.Sample(2.1f, true, out));
    CHECK(out.position.x == doctest::Approx(21.0f));
}

TEST_CASE("efenet::SnapshotBuffer congela al pasar kMaxExtrapolation") {
    // Sin tope, un jugador que se desconecta sale caminando al infinito.
    SnapshotBuffer buf;
    buf.Push(1.0f, en(10.0f));
    buf.Push(2.0f, en(20.0f));

    PlayerState out;
    REQUIRE(buf.Sample(2.0f + kMaxExtrapolation + 1.0f, true, out));

    // Se congela en el limite: 20 + 10 * kMaxExtrapolation.
    const f32 esperado = 20.0f + 10.0f * kMaxExtrapolation;
    CHECK(out.position.x == doctest::Approx(esperado));
}

TEST_CASE("efenet::SnapshotBuffer con extrapolacion apagada congela en el ultimo") {
    // El toggle del panel: apagarla tiene que dejar al avatar quieto en el
    // ultimo snapshot, no seguir de largo.
    SnapshotBuffer buf;
    buf.Push(1.0f, en(10.0f));
    buf.Push(2.0f, en(20.0f));

    PlayerState out;
    REQUIRE(buf.Sample(2.5f, false, out));
    CHECK(out.position.x == doctest::Approx(20.0f));
}

TEST_CASE("efenet::SnapshotBuffer ignora los que llegan fuera de orden") {
    // UDP entrega desordenado: un snapshot viejo no puede pisar uno nuevo.
    SnapshotBuffer buf;
    buf.Push(1.0f, en(10.0f));
    buf.Push(2.0f, en(20.0f));
    buf.Push(1.5f, en(999.0f));   // llega tarde, ya lo pasamos

    CHECK(buf.Count() == 2u);
    CHECK(buf.NewestTime() == doctest::Approx(2.0f));

    PlayerState out;
    REQUIRE(buf.Sample(2.0f, true, out));
    CHECK(out.position.x == doctest::Approx(20.0f));
}

TEST_CASE("efenet::SnapshotBuffer interpola el yaw por el camino corto") {
    SnapshotBuffer buf;
    buf.Push(1.0f, en(0.0f,  170.0f * DEG_TO_RAD));
    buf.Push(2.0f, en(0.0f, -170.0f * DEG_TO_RAD));

    PlayerState out;
    REQUIRE(buf.Sample(1.5f, true, out));

    CHECK(std::abs(std::abs(out.yaw) - PI) < 0.01f);
}

TEST_CASE("efenet::SnapshotBuffer sobrevive dar la vuelta al ring") {
    SnapshotBuffer buf;
    for (u32 i = 1u; i <= 200u; ++i) {
        buf.Push(static_cast<f32>(i), en(static_cast<f32>(i)));
    }

    PlayerState out;
    REQUIRE(buf.Sample(200.0f, true, out));
    CHECK(out.position.x == doctest::Approx(200.0f));
}

TEST_CASE("efenet::SnapshotBuffer Clear lo deja vacio") {
    SnapshotBuffer buf;
    buf.Push(1.0f, en(10.0f));
    buf.Clear();

    PlayerState out;
    CHECK(buf.Count() == 0u);
    CHECK_FALSE(buf.Sample(1.0f, true, out));
}
