#pragma once
#include <efenet/Transport.h>
#include <efenet/World.h>
#include <efenet/InputCmd.h>
#include <efenet/Protocol.h>

#include <array>
#include <deque>

namespace efenet {

    // El servidor authoritative. Headless: no incluye ni linkea nada grafico.
    //
    // Simula a kTickRate (60 Hz) y difunde a kSnapshotRate (20 Hz), o sea un
    // snapshot cada kTicksPerSnapshot ticks.
    class Server {
        public:
            explicit Server(u16 port);

            bool Ok() const { return m_transport.Ok(); }

            // Un paso completo: recibir, aplicar un input por cliente, simular
            // a todos, y cada 3 ticks difundir.
            void Tick();

            TickId CurrentTick() const { return m_tick; }
            usize  PlayerCount() const { return m_world.Count(); }
            const World& GetWorld() const { return m_world; }

        private:
            // Un cliente conectado, indexado por el numero de peer de ENet.
            struct Slot {
                bool     active = false;
                PlayerId player = kInvalidPlayer;

                // JITTER BUFFER. Los inputs no llegan parejos: llegan en
                // rafagas. Si el servidor aplicara en un tick todos los que
                // llegaron, ese jugador avanzaria a tirones. Se consume
                // exactamente UNO por tick.
                std::deque<InputCmd> queue;

                // Si la cola queda vacia (paquete perdido o tardio) se repite
                // este: el jugador sigue caminando en vez de trabarse.
                InputCmd lastApplied {};

                // El ultimo seq aplicado. Ancla la reconciliacion del cliente y
                // sirve para descartar los repetidos que trae la redundancia.
                u32 lastProcessedSeq = 0u;
            };

            // Si la cola pasa de esto, el cliente va adelantado: se consumen dos
            // en un tick para no acumular retraso permanente.
            static constexpr usize kQueueHighWater = 4u;

            void onConnect(u32 peer);
            void onDisconnect(u32 peer);
            void onPacket(u32 peer, const u8* data, usize size);
            void applyInputs();
            void broadcastSnapshots();

            glm::vec3 spawnPoint() const;

            Transport m_transport;
            World     m_world;
            TickId    m_tick = 0u;

            std::array<Slot, kMaxPlayers> m_slots {};
    };

}
