#pragma once
#include <efenet/Transport.h>
#include <efenet/Prediction.h>
#include <efenet/SnapshotBuffer.h>
#include <efenet/NetSim.h>
#include <efenet/Protocol.h>

#include <unordered_map>
#include <vector>

namespace efenet {

    // El cliente. Posee la prediccion del avatar propio y los buffers de
    // interpolacion de los remotos.
    //
    // Dos ritmos distintos, a proposito:
    //   Poll(dt)  -> CADA FRAME. Recibe y reconcilia. Si se llamara solo dentro
    //                del tick fijo, a 20 fps se llamaria rara vez y la latencia
    //                percibida subiria por un detalle de estructura del loop, no
    //                por la red.
    //   Tick(cmd) -> A 60 Hz EXACTOS. Predice y envia.
    class Client {
        public:
            Client();

            bool Ok() const { return m_transport.Ok(); }
            bool Connect(const char* host, u16 port);
            bool Connected() const { return m_connected; }

            void Poll(f32 dt);
            void Tick(const InputCmd& cmd);

            PlayerId           MyId()      const { return m_myId; }
            const PlayerState& Predicted() const { return m_predicted; }

            // Tiempo de servidor estimado, en segundos.
            f32 ServerTime() const { return m_serverTime; }

            // Llena 'out' con las poses de los remotos ya interpoladas para
            // 'renderTime'. NO incluye al jugador propio: ese va predicho.
            void SampleRemotes(f32 renderTime, std::vector<PlayerState>& out) const;

            // Los switches del demo. Apagar cada tecnica por separado es lo que
            // hace que el PoC demuestre algo.
            struct Toggles {
                bool prediction     = true;
                bool reconciliation = true;
                bool interpolation  = true;
                bool extrapolation  = true;
            };
            Toggles&       toggles()       { return m_toggles; }
            const Toggles& toggles() const { return m_toggles; }

            struct Stats {
                u32   rttMs              = 0u;
                u32   snapshotsReceived  = 0u;
                u32   snapshotsDiscarded = 0u;   // fuera de orden
                usize pendingInputs      = 0u;
                f32   lastReconcileError = 0.0f; // metrica de salud
                u32   bytesInPerSec      = 0u;
                u32   bytesOutPerSec     = 0u;
            };
            const Stats& GetStats() const { return m_stats; }

            NetSim& netSim() { return m_netSim; }

        private:
            void drenarTransporte(f32 now);
            void procesarPaquete(const u8* data, usize size);
            void aplicarSnapshot(const SnapshotMsg& msg);
            void enviar(const u8* data, usize size, bool reliable);
            void actualizarStats(f32 dt);

            Transport   m_transport;
            NetSim      m_netSim;
            Prediction  m_prediction;
            PlayerState m_predicted;

            std::unordered_map<PlayerId, SnapshotBuffer> m_remotes;

            PlayerId m_myId      = kInvalidPlayer;
            bool     m_connected = false;

            // Reloj local que estima el del servidor. Avanza con dt y se
            // corrige SUAVE hacia el ultimo snapshot: corregirlo de golpe
            // produce tirones visibles en todos los avatares a la vez.
            f32  m_serverTime      = 0.0f;
            bool m_serverTimeValido = false;

            TickId m_lastSnapshotTick = 0u;

            // Buffer reusado por Tick() para no realocar 60 veces por segundo.
            std::vector<InputCmd> m_scratchCommands;

            Toggles m_toggles;
            Stats   m_stats;

            f32 m_now             = 0.0f;   // reloj local acumulado, para NetSim
            f32 m_statsAcc        = 0.0f;
            u32 m_bytesInAcc      = 0u;
            u32 m_bytesOutAcc     = 0u;
    };

}
