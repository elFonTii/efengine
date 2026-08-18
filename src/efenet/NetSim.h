#pragma once
#include <efenet/Types.h>

#include <deque>
#include <random>
#include <vector>

namespace efenet {

    struct NetSimSettings {
        f32 latencyMs = 0.0f;   // 0..500
        f32 jitterMs  = 0.0f;   // 0..100
        f32 lossPct   = 0.0f;   // 0..30
    };

    // Simulador de red mala. Se mete entre Client y Transport.
    //
    // Sin esto el PoC no demuestra nada: en localhost la latencia es ~0 y la
    // prediccion es indistinguible de no tener prediccion. Con 200 ms de
    // latencia y los toggles del panel, se ve exactamente que aporta cada
    // tecnica de compensacion.
    //
    // Cada paquete se encola con deliverAt = now + latencia + rand(jitter), y
    // se drena por tiempo. Con jitter la cola puede querer entregar fuera de
    // orden: se permite, porque es lo que hace una red real y lo que el diseno
    // tiene que tolerar.
    class NetSim {
        public:
            NetSimSettings settings;

            bool Active() const;

            void PushOutbound(f32 now, const u8* data, usize size, bool reliable);
            void PushInbound (f32 now, u32 peer, const u8* data, usize size);

            // false cuando no queda nada vencido. El patron es un while(Pop...).
            bool PopOutbound(f32 now, std::vector<u8>& data, bool& reliable);
            bool PopInbound (f32 now, u32& peer, std::vector<u8>& data);

            usize PendingOutbound() const { return m_outbound.size(); }
            usize PendingInbound()  const { return m_inbound.size(); }
            u32   Dropped() const { return m_dropped; }

            void Clear();

        private:
            struct Packet {
                f32             deliverAt = 0.0f;
                u32             peer      = 0u;
                bool            reliable  = false;
                std::vector<u8> data;
            };

            // true si el dado de perdida dice que este paquete se tira. Los
            // confiables NUNCA se tiran: NetSim esta por encima de ENet, asi que
            // descartar un Welcome romperia el handshake en vez de simular una
            // red mala.
            bool shouldDrop(bool reliable);

            f32 deliveryTime(f32 now);

            std::deque<Packet> m_outbound;
            std::deque<Packet> m_inbound;

            // Semilla fija: dos corridas con la misma configuracion se
            // comportan igual, y un bug que aparece con 12% de perdida se puede
            // reproducir.
            std::mt19937 m_rng { 0xEFE7u };

            u32 m_dropped = 0u;
    };

}
