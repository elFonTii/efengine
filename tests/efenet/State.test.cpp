#include <doctest/doctest.h>
#include <efenet/PlayerState.h>
#include <efenet/InputCmd.h>
#include <efengine/serialization/BinaryWriter.h>
#include <efengine/serialization/BinaryReader.h>

using namespace efengine;
using namespace efenet;

/*
    Los tamaños serializados están fijados a propósito: son parte del contrato
    del protocolo, y la cuenta de bandwidth de la spec depende de ellos. Si
    alguien agrega un campo, estos CHECK se ponen rojos y obligan a subir
    kProtocolVersion en vez de romper la compatibilidad en silencio.
*/

TEST_CASE("efenet::PlayerState serializa 20 bytes y hace roundtrip") {
    PlayerState original;
    original.id       = 7u;
    original.position = glm::vec3(1.5f, -2.0f, 3.25f);
    original.yaw      = 1.25f;

    serialization::BinaryWriter w;
    original.Serialize(w);
    CHECK(w.Size() == 20u);

    PlayerState leido;
    serialization::BinaryReader r(w.Buffer());
    leido.Serialize(r);

    CHECK(r.Ok());
    CHECK(leido.id == 7u);
    CHECK(leido.position.x == doctest::Approx(1.5f));
    CHECK(leido.position.y == doctest::Approx(-2.0f));
    CHECK(leido.position.z == doctest::Approx(3.25f));
    CHECK(leido.yaw == doctest::Approx(1.25f));
}

TEST_CASE("efenet::InputCmd serializa 9 bytes y hace roundtrip") {
    InputCmd original;
    original.seq     = 42u;
    original.yaw     = -0.75f;
    original.buttons = Button::Forward | Button::Left;

    serialization::BinaryWriter w;
    original.Serialize(w);
    CHECK(w.Size() == 9u);

    InputCmd leido;
    serialization::BinaryReader r(w.Buffer());
    leido.Serialize(r);

    CHECK(r.Ok());
    CHECK(leido.seq == 42u);
    CHECK(leido.yaw == doctest::Approx(-0.75f));
    CHECK(leido.buttons == (Button::Forward | Button::Left));
}

TEST_CASE("efenet::PlayerState por defecto es invalido") {
    // kInvalidPlayer es 0 y los ids reales arrancan en 1, asi que un
    // PlayerState recien construido no se puede confundir con el jugador 0.
    PlayerState p;
    CHECK(p.id == kInvalidPlayer);
}

TEST_CASE("efenet::InputCmd truncado deja el reader en no-Ok") {
    serialization::BinaryWriter w;
    InputCmd original;
    original.seq = 1u;
    original.Serialize(w);

    std::vector<u8> cortado = w.Buffer();
    cortado.resize(cortado.size() - 1u);   // le falta el byte de buttons

    InputCmd leido;
    serialization::BinaryReader r(cortado);
    leido.Serialize(r);

    CHECK_FALSE(r.Ok());
}
