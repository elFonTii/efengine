#include <doctest/doctest.h>
#include <efenet/NetSim.h>

#include <vector>

using namespace efenet;

/*
    NetSim se mete entre Client y Transport para simular una red mala en
    localhost. Es tooling, pero es tooling testeable: las colas por tiempo y el
    dado de perdida son logica pura.
*/

namespace {
    const u8 kPayload[] = { 1u, 2u, 3u, 4u };

    void empujarSalida(NetSim& sim, f32 now) {
        sim.PushOutbound(now, kPayload, sizeof(kPayload), false);
    }
}

TEST_CASE("efenet::NetSim apagado no esta activo") {
    NetSim sim;
    CHECK_FALSE(sim.Active());

    sim.settings.latencyMs = 50.0f;
    CHECK(sim.Active());
}

TEST_CASE("efenet::NetSim sin latencia entrega en el acto") {
    NetSim sim;
    empujarSalida(sim, 0.0f);

    std::vector<u8> data;
    bool reliable = true;
    CHECK(sim.PopOutbound(0.0f, data, reliable));
    REQUIRE(data.size() == 4u);
    CHECK(data[0] == 1u);
    CHECK_FALSE(reliable);
}

TEST_CASE("efenet::NetSim con latencia retiene hasta que vence") {
    NetSim sim;
    sim.settings.latencyMs = 100.0f;
    empujarSalida(sim, 0.0f);

    std::vector<u8> data;
    bool reliable = false;

    // Antes de los 100 ms no sale nada.
    CHECK_FALSE(sim.PopOutbound(0.050f, data, reliable));
    CHECK(sim.PendingOutbound() == 1u);

    // Pasados los 100 ms, sale.
    CHECK(sim.PopOutbound(0.110f, data, reliable));
    CHECK(sim.PendingOutbound() == 0u);
}

TEST_CASE("efenet::NetSim con 100% de perdida no entrega nada no confiable") {
    NetSim sim;
    sim.settings.lossPct = 100.0f;

    for (u32 i = 0u; i < 20u; ++i) empujarSalida(sim, 0.0f);

    std::vector<u8> data;
    bool reliable = false;
    CHECK_FALSE(sim.PopOutbound(10.0f, data, reliable));
    CHECK(sim.Dropped() == 20u);
}

TEST_CASE("efenet::NetSim NUNCA descarta paquetes confiables") {
    // ENet retransmitiria los confiables, pero NetSim esta POR ENCIMA de ENet:
    // si tirara el Welcome, el handshake nunca se completaria y el demo no
    // arrancaria con la perdida al maximo. Solo se retrasan.
    NetSim sim;
    sim.settings.lossPct = 100.0f;

    sim.PushOutbound(0.0f, kPayload, sizeof(kPayload), true);

    std::vector<u8> data;
    bool reliable = false;
    CHECK(sim.PopOutbound(1.0f, data, reliable));
    CHECK(reliable);
}

TEST_CASE("efenet::NetSim con 0% de perdida entrega todo") {
    NetSim sim;
    sim.settings.lossPct = 0.0f;

    for (u32 i = 0u; i < 20u; ++i) empujarSalida(sim, 0.0f);

    std::vector<u8> data;
    bool reliable = false;
    u32 recibidos = 0u;
    while (sim.PopOutbound(1.0f, data, reliable)) ++recibidos;

    CHECK(recibidos == 20u);
    CHECK(sim.Dropped() == 0u);
}

TEST_CASE("efenet::NetSim las colas de entrada y salida son independientes") {
    NetSim sim;
    sim.PushInbound(0.0f, 3u, kPayload, sizeof(kPayload));

    CHECK(sim.PendingInbound() == 1u);
    CHECK(sim.PendingOutbound() == 0u);

    u32 peer = 0u;
    std::vector<u8> data;
    CHECK(sim.PopInbound(0.0f, peer, data));
    CHECK(peer == 3u);
}

TEST_CASE("efenet::NetSim el jitter mantiene la entrega dentro de la ventana") {
    // Con latencia 100 y jitter 50, todo tiene que entregarse entre 100 y 150
    // ms. Nada antes de los 100.
    NetSim sim;
    sim.settings.latencyMs = 100.0f;
    sim.settings.jitterMs  = 50.0f;

    for (u32 i = 0u; i < 50u; ++i) empujarSalida(sim, 0.0f);

    std::vector<u8> data;
    bool reliable = false;

    // Nada antes del piso de latencia.
    CHECK_FALSE(sim.PopOutbound(0.099f, data, reliable));

    // Todo entregado pasado el techo.
    u32 recibidos = 0u;
    while (sim.PopOutbound(0.151f, data, reliable)) ++recibidos;
    CHECK(recibidos == 50u);
}

TEST_CASE("efenet::NetSim Clear vacia las dos colas") {
    NetSim sim;
    sim.settings.latencyMs = 500.0f;
    empujarSalida(sim, 0.0f);
    sim.PushInbound(0.0f, 1u, kPayload, sizeof(kPayload));

    sim.Clear();

    CHECK(sim.PendingOutbound() == 0u);
    CHECK(sim.PendingInbound() == 0u);
}
