#include <doctest/doctest.h>
#include <efenet/Protocol.h>

#include <vector>

using namespace efenet;

/*
    Los paquetes son datos de AFUERA: pueden llegar truncados, corruptos, o
    directamente fabricados. Ningun Read* puede leer fuera de rango ni devolver
    true con basura adentro.

    Los primeros tests verifican el camino feliz; los de abajo, que el parser
    aguante lo que no lo es.
*/

TEST_CASE("efenet::Protocol Welcome hace roundtrip") {
    WelcomeMsg original;
    original.protocolVersion = kProtocolVersion;
    original.yourId          = 3u;
    original.serverTick      = 1234u;

    const std::vector<u8> bytes = WriteWelcome(original);
    CHECK(bytes.size() == 11u);   // msgId + u16 + u32 + u32

    MsgId id = MsgId::Input;
    CHECK(PeekMsgId(bytes.data(), bytes.size(), id));
    CHECK(id == MsgId::Welcome);

    WelcomeMsg leido;
    CHECK(ReadWelcome(bytes.data(), bytes.size(), leido));
    CHECK(leido.protocolVersion == kProtocolVersion);
    CHECK(leido.yourId == 3u);
    CHECK(leido.serverTick == 1234u);
}

TEST_CASE("efenet::Protocol Input hace roundtrip con redundancia") {
    InputMsg original;
    for (u32 i = 1u; i <= kInputRedundancy; ++i) {
        InputCmd c;
        c.seq     = i;
        c.yaw     = static_cast<f32>(i) * 0.25f;
        c.buttons = Button::Forward;
        original.commands.push_back(c);
    }

    const std::vector<u8> bytes = WriteInput(original);
    CHECK(bytes.size() == 2u + kInputRedundancy * 9u);   // 1+1+27 = 29

    InputMsg leido;
    CHECK(ReadInput(bytes.data(), bytes.size(), leido));
    REQUIRE(leido.commands.size() == kInputRedundancy);
    CHECK(leido.commands[0].seq == 1u);
    CHECK(leido.commands[2].seq == 3u);
    CHECK(leido.commands[2].yaw == doctest::Approx(0.75f));
}

TEST_CASE("efenet::Protocol Snapshot hace roundtrip con 8 jugadores") {
    SnapshotMsg original;
    original.serverTick   = 900u;
    original.lastInputSeq = 77u;
    for (u32 i = 1u; i <= kMaxPlayers; ++i) {
        PlayerState p;
        p.id       = i;
        p.position = glm::vec3(static_cast<f32>(i), 0.0f, -static_cast<f32>(i));
        p.yaw      = static_cast<f32>(i) * 0.1f;
        original.players.push_back(p);
    }

    const std::vector<u8> bytes = WriteSnapshot(original);
    // msgId + tick + lastInputSeq + count + 8*20 = 1+4+4+1+160 = 170.
    // Es el numero del que sale la cuenta de bandwidth de la spec: si alguien
    // agrega un campo, este CHECK obliga a subir kProtocolVersion.
    CHECK(bytes.size() == 170u);

    SnapshotMsg leido;
    CHECK(ReadSnapshot(bytes.data(), bytes.size(), leido));
    CHECK(leido.serverTick == 900u);
    CHECK(leido.lastInputSeq == 77u);
    REQUIRE(leido.players.size() == kMaxPlayers);
    CHECK(leido.players[7].id == 8u);
    CHECK(leido.players[7].position.x == doctest::Approx(8.0f));
}

TEST_CASE("efenet::Protocol Snapshot vacio es valido") {
    // Un servidor sin jugadores difunde igual: el cliente tiene que poder
    // parsear un snapshot de cero jugadores sin tratarlo como error.
    SnapshotMsg original;
    original.serverTick = 5u;

    const std::vector<u8> bytes = WriteSnapshot(original);
    SnapshotMsg leido;

    CHECK(ReadSnapshot(bytes.data(), bytes.size(), leido));
    CHECK(leido.players.empty());
}

TEST_CASE("efenet::Protocol un buffer vacio no tiene msgId") {
    MsgId id = MsgId::Welcome;
    CHECK_FALSE(PeekMsgId(null, 0u, id));
}

TEST_CASE("efenet::Protocol rechaza un msgId desconocido") {
    const u8 bytes[] = { 99u, 0u, 0u, 0u };
    MsgId id = MsgId::Welcome;
    CHECK_FALSE(PeekMsgId(bytes, sizeof(bytes), id));
}

TEST_CASE("efenet::Protocol rechaza un Snapshot truncado en cualquier corte") {
    SnapshotMsg original;
    original.serverTick = 1u;
    for (u32 i = 1u; i <= 4u; ++i) {
        PlayerState p;
        p.id = i;
        original.players.push_back(p);
    }
    const std::vector<u8> completo = WriteSnapshot(original);

    // Barrido: cortar el buffer en CADA longitud posible. Ninguna puede
    // devolver true, y ninguna puede leer fuera de rango.
    for (usize corte = 0u; corte < completo.size(); ++corte) {
        SnapshotMsg leido;
        CHECK_FALSE(ReadSnapshot(completo.data(), corte, leido));
    }

    SnapshotMsg leido;
    CHECK(ReadSnapshot(completo.data(), completo.size(), leido));
}

TEST_CASE("efenet::Protocol rechaza un Input con count mentiroso") {
    // El count dice 200 comandos pero el buffer trae uno. Sin validacion, el
    // reader intentaria leer 1800 bytes de un buffer de 11.
    std::vector<u8> bytes;
    bytes.push_back(static_cast<u8>(MsgId::Input));
    bytes.push_back(200u);
    for (u32 i = 0u; i < 9u; ++i) bytes.push_back(0u);

    InputMsg leido;
    CHECK_FALSE(ReadInput(bytes.data(), bytes.size(), leido));
}

TEST_CASE("efenet::Protocol rechaza un Input con count sobre el limite") {
    // Entra en el buffer, pero kInputRedundancy es 3: un cliente que manda 10
    // esta fuera de contrato y no se procesa.
    InputMsg abusivo;
    for (u32 i = 1u; i <= 10u; ++i) {
        InputCmd c;
        c.seq = i;
        abusivo.commands.push_back(c);
    }
    const std::vector<u8> bytes = WriteInput(abusivo);

    InputMsg leido;
    CHECK_FALSE(ReadInput(bytes.data(), bytes.size(), leido));
}

TEST_CASE("efenet::Protocol rechaza un Snapshot con mas jugadores que kMaxPlayers") {
    SnapshotMsg abusivo;
    for (u32 i = 1u; i <= kMaxPlayers + 5u; ++i) {
        PlayerState p;
        p.id = i;
        abusivo.players.push_back(p);
    }
    const std::vector<u8> bytes = WriteSnapshot(abusivo);

    SnapshotMsg leido;
    CHECK_FALSE(ReadSnapshot(bytes.data(), bytes.size(), leido));
}

TEST_CASE("efenet::Protocol leer un mensaje con el parser equivocado falla") {
    WelcomeMsg w;
    w.yourId = 1u;
    const std::vector<u8> bytes = WriteWelcome(w);

    SnapshotMsg leido;
    CHECK_FALSE(ReadSnapshot(bytes.data(), bytes.size(), leido));
}
