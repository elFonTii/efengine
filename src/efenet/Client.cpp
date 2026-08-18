#include <efenet/Client.h>
#include <efenet/Simulation.h>

#include <efengine/core/Log.h>

#include <cmath>
#include <iterator>

namespace efenet {

namespace {
    // Fraccion del error de reloj que se corrige por frame. Chico a proposito:
    // saltar de golpe al tiempo del servidor hace temblar a todos los remotos.
    constexpr f32 kClockCorrectionRate = 0.1f;

    // En modo cliente el host se crea con un solo peer, asi que el servidor es
    // siempre el indice 0.
    constexpr u32 kServerPeer = 0u;
}

    Client::Client()
        : m_transport(0u, 1u) {}   // sin puerto de escucha, un solo peer

    bool Client::Connect(const char* host, u16 port) {
        if (!m_transport.Ok()) return false;
        return m_transport.Connect(host, port);
    }

    void Client::enviar(const u8* data, usize size, bool reliable) {
        m_bytesOutAcc += static_cast<u32>(size);

        if (m_netSim.Active()) {
            m_netSim.PushOutbound(m_now, data, size, reliable);
            return;
        }
        m_transport.Send(kServerPeer, data, size, reliable);
    }

    void Client::drenarTransporte(f32 now) {
        Event ev;
        while (m_transport.Poll(ev)) {
            switch (ev.type) {
                case EventType::Connect:
                    m_connected = true;
                    EF_LOG_INFO("efenet: conectado al servidor");
                    break;

                case EventType::Disconnect:
                    m_connected = false;
                    m_myId      = kInvalidPlayer;
                    m_remotes.clear();
                    m_prediction.Clear();
                    m_serverTimeValido = false;
                    EF_LOG_INFO("efenet: desconectado del servidor");
                    break;

                case EventType::Receive:
                    m_bytesInAcc += static_cast<u32>(ev.size);
                    if (m_netSim.Active()) {
                        // Copia obligatoria: ev.data muere en el proximo Poll.
                        m_netSim.PushInbound(now, ev.peer, ev.data, ev.size);
                    } else {
                        procesarPaquete(ev.data, ev.size);
                    }
                    break;

                default:
                    break;
            }
        }

        // Drenar lo que el simulador ya dejo vencer, en ambas direcciones.
        if (m_netSim.Active()) {
            std::vector<u8> data;
            bool reliable = false;
            while (m_netSim.PopOutbound(now, data, reliable)) {
                m_transport.Send(kServerPeer, data.data(), data.size(), reliable);
            }

            u32 peer = 0u;
            while (m_netSim.PopInbound(now, peer, data)) {
                procesarPaquete(data.data(), data.size());
            }
        }
    }

    void Client::procesarPaquete(const u8* data, usize size) {
        MsgId id = MsgId::Snapshot;
        if (!PeekMsgId(data, size, id)) return;

        if (id == MsgId::Welcome) {
            WelcomeMsg msg;
            if (!ReadWelcome(data, size, msg)) return;

            if (msg.protocolVersion != kProtocolVersion) {
                EF_LOG_ERROR("efenet: version de protocolo incompatible (servidor %u, cliente %u)",
                             static_cast<u32>(msg.protocolVersion),
                             static_cast<u32>(kProtocolVersion));
                m_transport.Disconnect(kServerPeer);
                return;
            }

            m_myId = msg.yourId;
            m_serverTime       = static_cast<f32>(msg.serverTick) * kTickDt;
            m_serverTimeValido = true;
            EF_LOG_INFO("efenet: sos el jugador %u", m_myId);
            return;
        }

        if (id == MsgId::Snapshot) {
            SnapshotMsg msg;
            if (!ReadSnapshot(data, size, msg)) return;
            aplicarSnapshot(msg);
        }
    }

    void Client::aplicarSnapshot(const SnapshotMsg& msg) {
        // UDP entrega fuera de orden: un snapshot viejo no puede pisar uno nuevo.
        if (m_serverTimeValido && msg.serverTick <= m_lastSnapshotTick) {
            ++m_stats.snapshotsDiscarded;
            return;
        }
        m_lastSnapshotTick = msg.serverTick;
        ++m_stats.snapshotsReceived;

        const f32 tiempoDelSnapshot = static_cast<f32>(msg.serverTick) * kTickDt;

        // Correccion SUAVE del reloj estimado.
        if (!m_serverTimeValido) {
            m_serverTime       = tiempoDelSnapshot;
            m_serverTimeValido = true;
        } else {
            m_serverTime += (tiempoDelSnapshot - m_serverTime) * kClockCorrectionRate;
        }

        // El propio: reconciliar. Los ajenos: al buffer de interpolacion.
        std::vector<PlayerId> presentes;
        presentes.reserve(msg.players.size());

        for (const PlayerState& p : msg.players) {
            presentes.push_back(p.id);

            if (p.id == m_myId) {
                if (!m_toggles.reconciliation) {
                    // Toggle apagado: la prediccion sigue sola y deriva. Es
                    // justamente lo que el demo quiere hacer visible.
                    continue;
                }

                const glm::vec3 antes = m_predicted.position;
                m_prediction.Reconcile(m_predicted, p, msg.lastInputSeq);
                m_stats.lastReconcileError = glm::length(m_predicted.position - antes);
                continue;
            }

            m_remotes[p.id].Push(tiempoDelSnapshot, p);
        }

        // MEMBRESIA IMPLICITA: quien no vino en el snapshot se fue de la
        // partida. No hacen falta paquetes de join/leave.
        for (auto it = m_remotes.begin(); it != m_remotes.end(); ) {
            bool sigue = false;
            for (PlayerId id : presentes) {
                if (id == it->first) { sigue = true; break; }
            }
            it = sigue ? std::next(it) : m_remotes.erase(it);
        }
    }

    void Client::Poll(f32 dt) {
        m_now += dt;

        // El reloj estimado avanza con el dt local entre snapshots.
        if (m_serverTimeValido) m_serverTime += dt;

        drenarTransporte(m_now);
        actualizarStats(dt);
    }

    void Client::Tick(const InputCmd& cmd) {
        if (!m_connected || m_myId == kInvalidPlayer) return;

        m_prediction.Push(cmd);

        // PREDICCION: aplicar Step() ya, sin esperar al servidor. El movimiento
        // responde en el frame en vez de en RTT milisegundos.
        if (m_toggles.prediction) Step(m_predicted, cmd, kTickDt);

        // Con REDUNDANCIA: los ultimos 3 comandos, no solo el nuevo. Si se
        // pierde este paquete, el siguiente ya trae el comando que falto.
        InputMsg msg;
        m_prediction.LastN(kInputRedundancy, m_scratchCommands);
        msg.commands = m_scratchCommands;

        const std::vector<u8> bytes = WriteInput(msg);
        enviar(bytes.data(), bytes.size(), false);   // no confiable
    }

    void Client::SampleRemotes(f32 renderTime, std::vector<PlayerState>& out) const {
        out.clear();
        for (const auto& par : m_remotes) {
            PlayerState pose;

            if (!m_toggles.interpolation) {
                // Toggle apagado: el ultimo snapshot crudo. Se ve saltar a 20 Hz.
                if (!par.second.Sample(par.second.NewestTime(), false, pose)) continue;
            } else {
                if (!par.second.Sample(renderTime, m_toggles.extrapolation, pose)) continue;
            }

            out.push_back(pose);
        }
    }

    void Client::actualizarStats(f32 dt) {
        m_stats.rttMs         = m_transport.RttMs(kServerPeer);
        m_stats.pendingInputs = m_prediction.Pending();

        m_statsAcc += dt;
        if (m_statsAcc >= 1.0f) {
            m_stats.bytesInPerSec  = m_bytesInAcc;
            m_stats.bytesOutPerSec = m_bytesOutAcc;
            m_bytesInAcc  = 0u;
            m_bytesOutAcc = 0u;
            m_statsAcc    = 0.0f;
        }
    }

}
