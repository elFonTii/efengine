#include <efenet/Server.h>
#include <efenet/Simulation.h>

#include <efengine/core/Log.h>

#include <cmath>

namespace efenet {

    Server::Server(u16 port)
        : m_transport(port, kMaxPlayers) {
        if (m_transport.Ok()) {
            EF_LOG_INFO("efenet: servidor escuchando en el puerto %u", static_cast<u32>(port));
        }
    }

    glm::vec3 Server::spawnPoint() const {
        // Repartidos en circulo para que no aparezcan uno encima del otro.
        const f32 angulo = static_cast<f32>(m_world.Count()) * (TAU / static_cast<f32>(kMaxPlayers));
        const f32 radio  = 5.0f;
        return glm::vec3(std::cos(angulo) * radio, 0.0f, std::sin(angulo) * radio);
    }

    void Server::onConnect(u32 peer) {
        if (peer >= kMaxPlayers) return;

        if (m_world.Full()) {
            EF_LOG_INFO("efenet: partida llena, se rechaza el peer %u", peer);
            m_transport.Disconnect(peer);
            return;
        }

        Slot& slot = m_slots[peer];
        slot = Slot {};
        slot.active = true;
        slot.player = m_world.Add(spawnPoint());

        WelcomeMsg welcome;
        welcome.protocolVersion = kProtocolVersion;
        welcome.yourId          = slot.player;
        welcome.serverTick      = m_tick;

        const std::vector<u8> bytes = WriteWelcome(welcome);
        m_transport.Send(peer, bytes.data(), bytes.size(), true);   // confiable

        EF_LOG_INFO("efenet: jugador %u entro (peer %u), %u en partida",
                    slot.player, peer, static_cast<u32>(m_world.Count()));
    }

    void Server::onDisconnect(u32 peer) {
        if (peer >= kMaxPlayers) return;

        Slot& slot = m_slots[peer];
        if (!slot.active) return;

        EF_LOG_INFO("efenet: jugador %u salio", slot.player);
        m_world.Remove(slot.player);
        slot = Slot {};
        // Los demas clientes se enteran solos: deja de aparecer en el snapshot.
    }

    void Server::onPacket(u32 peer, const u8* data, usize size) {
        if (peer >= kMaxPlayers) return;
        Slot& slot = m_slots[peer];
        if (!slot.active) return;

        MsgId id = MsgId::Input;
        if (!PeekMsgId(data, size, id)) return;   // basura: se ignora
        if (id != MsgId::Input) return;           // el cliente no manda otra cosa

        InputMsg msg;
        if (!ReadInput(data, size, msg)) return;  // truncado o mentiroso

        for (const InputCmd& cmd : msg.commands) {
            // La redundancia hace que lleguen repetidos: los ya aplicados se
            // descartan. Tambien filtra los que llegan fuera de orden.
            if (cmd.seq <= slot.lastProcessedSeq) continue;

            // Y los que ya estan encolados, por si el mismo comando llego en
            // dos paquetes distintos.
            if (!slot.queue.empty() && cmd.seq <= slot.queue.back().seq) continue;

            slot.queue.push_back(cmd);
        }
    }

    void Server::applyInputs() {
        for (Slot& slot : m_slots) {
            if (!slot.active) continue;

            PlayerState* state = m_world.Find(slot.player);
            if (state == null) continue;

            // Cola larga: el cliente va adelantado. Consumir dos evita que el
            // retraso se vuelva permanente.
            usize aConsumir = 1u;
            if (slot.queue.size() > kQueueHighWater) aConsumir = 2u;

            for (usize i = 0u; i < aConsumir; ++i) {
                if (slot.queue.empty()) {
                    // Cola vacia: repetir el ultimo. El jugador sigue caminando
                    // en vez de trabarse por un paquete perdido.
                    Step(*state, slot.lastApplied, kTickDt);
                    break;
                }

                const InputCmd cmd = slot.queue.front();
                slot.queue.pop_front();

                Step(*state, cmd, kTickDt);
                slot.lastApplied      = cmd;
                slot.lastProcessedSeq = cmd.seq;
            }
        }
    }

    void Server::broadcastSnapshots() {
        // El snapshot se serializa UNA VEZ POR CLIENTE porque lastInputSeq es
        // distinto para cada uno: es lo que ancla su reconciliacion.
        SnapshotMsg msg;
        msg.serverTick = m_tick;
        msg.players    = m_world.Players();

        for (u32 peer = 0u; peer < kMaxPlayers; ++peer) {
            const Slot& slot = m_slots[peer];
            if (!slot.active) continue;

            msg.lastInputSeq = slot.lastProcessedSeq;
            const std::vector<u8> bytes = WriteSnapshot(msg);
            m_transport.Send(peer, bytes.data(), bytes.size(), false);   // no confiable
        }
    }

    void Server::Tick() {
        Event ev;
        while (m_transport.Poll(ev)) {
            switch (ev.type) {
                case EventType::Connect:    onConnect(ev.peer); break;
                case EventType::Disconnect: onDisconnect(ev.peer); break;
                case EventType::Receive:    onPacket(ev.peer, ev.data, ev.size); break;
                default: break;
            }
        }

        applyInputs();
        ++m_tick;

        if (m_tick % kTicksPerSnapshot == 0u) broadcastSnapshots();
    }

}
