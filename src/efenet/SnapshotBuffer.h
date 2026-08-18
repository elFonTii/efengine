#pragma once
#include <efenet/PlayerState.h>

#include <array>

namespace efenet {

    // Interpolacion de angulos por el CAMINO CORTO, en radianes.
    //
    // El lerp directo de angulos cruza mal el borde +-PI: interpolar de 170 a
    // -170 grados daria una vuelta de 340 grados en vez de 20, y el avatar
    // remoto pega un giro completo al reves. Es publica porque se testea sola.
    f32 LerpAngle(f32 a, f32 b, f32 t);

    // Guarda los ultimos snapshots de UN jugador remoto y devuelve su pose para
    // un instante dado.
    //
    // A los remotos no se los puede predecir: no tenemos sus inputs. Se los
    // dibuja kInterpDelay EN EL PASADO, interpolando entre los dos snapshots
    // que rodean ese instante. Con snapshots cada 50 ms, 100 ms de retardo deja
    // siempre dos disponibles y aguanta perder uno.
    class SnapshotBuffer {
        public:
            // 'serverTime' en segundos (serverTick * kTickDt). Los que llegan
            // fuera de orden (serverTime <= el ultimo) se ignoran: UDP entrega
            // desordenado y un snapshot viejo no puede pisar uno nuevo.
            void Push(f32 serverTime, const PlayerState& state);

            // Devuelve false solo si el buffer esta vacio.
            //
            // - renderTime antes del mas viejo -> se clampea al mas viejo
            // - renderTime entre dos           -> interpolacion
            // - renderTime despues del ultimo  -> extrapolacion con la velocidad
            //   derivada de los dos ultimos, con tope kMaxExtrapolation
            //
            // 'allowExtrapolation' es el toggle del panel de debug: en false, el
            // avatar se congela en el ultimo snapshot en vez de seguir de largo.
            bool Sample(f32 renderTime, bool allowExtrapolation, PlayerState& out) const;

            usize Count() const { return m_count; }
            f32   NewestTime() const;
            void  Clear();

        private:
            struct Entry {
                f32         time = 0.0f;
                PlayerState state {};
            };

            // 32 entradas a 20 Hz = 1,6 s de historia. De sobra para 100 ms de
            // retardo mas una racha de perdida.
            static constexpr usize kCapacity = 32u;

            std::array<Entry, kCapacity> m_ring {};
            usize m_head  = 0u;
            usize m_count = 0u;

            const Entry& at(usize i) const;   // i = 0 es el mas viejo
    };

}
