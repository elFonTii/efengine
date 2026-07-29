#include <doctest/doctest.h>
#include <efenet/Prediction.h>
#include <efenet/Simulation.h>

#include <vector>

using namespace efenet;

/*
    La reconciliacion es lo que hace que la prediccion no derive. La propiedad
    central: si el servidor y la prediccion coincidian, reconciliar NO tiene que
    mover nada. Ese es el primer test, y es el que se pone rojo si Step() deja
    de estar realmente compartido entre los dos lados.
*/

namespace {
    InputCmd cmd(u32 seq, u8 buttons, f32 yaw = 0.0f) {
        InputCmd c;
        c.seq     = seq;
        c.yaw     = yaw;
        c.buttons = buttons;
        return c;
    }
}

TEST_CASE("efenet::Prediction cuando el servidor coincide, reconciliar no mueve nada") {
    Prediction pred;
    PlayerState local;

    // El cliente predice 5 ticks.
    for (u32 i = 1u; i <= 5u; ++i) {
        const InputCmd c = cmd(i, Button::Forward);
        pred.Push(c);
        Step(local, c, kTickDt);
    }

    // El servidor proceso hasta el 3 y llego a la misma posicion, aplicando los
    // mismos inputs sobre el mismo estado inicial.
    PlayerState authoritative;
    for (u32 i = 1u; i <= 3u; ++i) Step(authoritative, cmd(i, Button::Forward), kTickDt);

    const PlayerState antes = local;
    pred.Reconcile(local, authoritative, 3u);

    CHECK(local.position.x == doctest::Approx(antes.position.x));
    CHECK(local.position.z == doctest::Approx(antes.position.z));
}

TEST_CASE("efenet::Prediction cuando el servidor discrepa, gana el servidor") {
    Prediction pred;
    PlayerState local;

    for (u32 i = 1u; i <= 5u; ++i) {
        const InputCmd c = cmd(i, Button::Forward);
        pred.Push(c);
        Step(local, c, kTickDt);
    }

    // El servidor dice que en el tick 3 estabas en otro lado.
    PlayerState authoritative;
    authoritative.position = glm::vec3(10.0f, 0.0f, 10.0f);

    pred.Reconcile(local, authoritative, 3u);

    // Corregido = la verdad del servidor + los inputs 4 y 5 re-aplicados.
    PlayerState esperado = authoritative;
    Step(esperado, cmd(4u, Button::Forward), kTickDt);
    Step(esperado, cmd(5u, Button::Forward), kTickDt);

    CHECK(local.position.x == doctest::Approx(esperado.position.x));
    CHECK(local.position.z == doctest::Approx(esperado.position.z));
}

TEST_CASE("efenet::Prediction Reconcile descarta los inputs ya confirmados") {
    Prediction pred;
    for (u32 i = 1u; i <= 5u; ++i) pred.Push(cmd(i, Button::Forward));
    CHECK(pred.Pending() == 5u);

    PlayerState local;
    PlayerState authoritative;
    pred.Reconcile(local, authoritative, 3u);

    CHECK(pred.Pending() == 2u);   // quedan el 4 y el 5
}

TEST_CASE("efenet::Prediction DiscardUpTo con un seq mayor a todos vacia el historial") {
    Prediction pred;
    for (u32 i = 1u; i <= 5u; ++i) pred.Push(cmd(i, Button::Forward));

    pred.DiscardUpTo(99u);
    CHECK(pred.Pending() == 0u);
}

TEST_CASE("efenet::Prediction DiscardUpTo con un seq viejo no descarta nada") {
    // UDP puede entregar fuera de orden: un ack viejo no puede borrar inputs
    // nuevos ni resucitar los ya descartados.
    Prediction pred;
    for (u32 i = 10u; i <= 15u; ++i) pred.Push(cmd(i, Button::Forward));

    pred.DiscardUpTo(3u);
    CHECK(pred.Pending() == 6u);
}

TEST_CASE("efenet::Prediction LastN devuelve los ultimos, del mas viejo al mas nuevo") {
    Prediction pred;
    for (u32 i = 1u; i <= 10u; ++i) pred.Push(cmd(i, Button::Forward));

    std::vector<InputCmd> ultimos;
    pred.LastN(3u, ultimos);

    REQUIRE(ultimos.size() == 3u);
    CHECK(ultimos[0].seq == 8u);
    CHECK(ultimos[1].seq == 9u);
    CHECK(ultimos[2].seq == 10u);
}

TEST_CASE("efenet::Prediction LastN con menos comandos de los pedidos devuelve los que hay") {
    // Los primeros ticks despues de conectar: todavia no hay 3 comandos.
    Prediction pred;
    pred.Push(cmd(1u, Button::Forward));

    std::vector<InputCmd> ultimos;
    pred.LastN(kInputRedundancy, ultimos);

    REQUIRE(ultimos.size() == 1u);
    CHECK(ultimos[0].seq == 1u);
}

TEST_CASE("efenet::Prediction LastN limpia el vector de salida antes de llenarlo") {
    // El cliente reusa el mismo vector cada tick para no realocar; si LastN no
    // limpiara, el paquete iria creciendo con comandos viejos.
    Prediction pred;
    pred.Push(cmd(7u, Button::Forward));

    std::vector<InputCmd> reusado;
    reusado.push_back(cmd(99u, 0u));
    reusado.push_back(cmd(98u, 0u));

    pred.LastN(kInputRedundancy, reusado);

    REQUIRE(reusado.size() == 1u);
    CHECK(reusado[0].seq == 7u);
}

TEST_CASE("efenet::Prediction el ring buffer sobrevive dar la vuelta") {
    // Mas empujes que capacidad: los viejos se pisan, los ultimos sobreviven.
    Prediction pred;
    const u32 total = kInputHistory * 2u + 7u;
    for (u32 i = 1u; i <= total; ++i) pred.Push(cmd(i, Button::Forward));

    CHECK(pred.Pending() == kInputHistory);

    std::vector<InputCmd> ultimos;
    pred.LastN(2u, ultimos);
    REQUIRE(ultimos.size() == 2u);
    CHECK(ultimos[1].seq == total);
}

TEST_CASE("efenet::Prediction reconciliar con historial vacio deja el estado del servidor") {
    Prediction pred;
    PlayerState local;
    local.position = glm::vec3(1.0f, 0.0f, 1.0f);

    PlayerState authoritative;
    authoritative.position = glm::vec3(5.0f, 0.0f, -5.0f);

    pred.Reconcile(local, authoritative, 0u);

    CHECK(local.position.x == doctest::Approx(5.0f));
    CHECK(local.position.z == doctest::Approx(-5.0f));
}

TEST_CASE("efenet::Prediction Clear vacia el historial") {
    Prediction pred;
    for (u32 i = 1u; i <= 5u; ++i) pred.Push(cmd(i, Button::Forward));

    pred.Clear();
    CHECK(pred.Pending() == 0u);
}
