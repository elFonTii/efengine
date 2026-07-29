#pragma once
#include <efenet/InputCmd.h>
#include <efenet/PlayerState.h>

#include <array>
#include <vector>

namespace efenet {

    // Historial de inputs sin confirmar + reconciliacion.
    //
    // El cliente predice su propio avatar aplicando Step() apenas muestrea el
    // input, sin esperar al servidor: el movimiento responde en el frame y no
    // en RTT milisegundos. Pero entonces cliente y servidor tienen dos
    // versiones del mismo avatar, y hay que volver a juntarlas.
    //
    // Reconcile() hace eso: pisa el estado local con el authoritative y
    // re-aplica los inputs que el servidor todavia no habia procesado. Si los
    // dos lados coincidian, el resultado es identico al que ya habia y no se ve
    // nada. Si no, el avatar salta a la posicion corregida, y ESE salto es la
    // metrica de salud que grafica el panel de debug.
    class Prediction {
        public:
            void Push(const InputCmd& cmd);

            // Copia los ultimos n comandos (o menos si hay menos), del mas
            // viejo al mas nuevo. 'out' se limpia primero: el cliente reusa el
            // mismo vector cada tick para no realocar.
            void LastN(u32 n, std::vector<InputCmd>& out) const;

            // Descarta todo comando con seq <= seq. Un seq viejo (snapshot
            // fuera de orden) no descarta nada.
            void DiscardUpTo(u32 seq);

            // RECONCILIACION. 'lastProcessedSeq' es el ultimo input de este
            // cliente que el servidor aplico, y viene en el snapshot.
            void Reconcile(PlayerState& state, const PlayerState& authoritative,
                           u32 lastProcessedSeq);

            usize Pending() const { return m_count; }
            void  Clear();

        private:
            // Ring buffer: a 60 Hz, kInputHistory da ~2 s de inputs sin
            // confirmar. Si se llena (servidor mudo), el mas viejo se pisa.
            std::array<InputCmd, kInputHistory> m_ring {};
            usize m_head  = 0u;   // proxima posicion a escribir
            usize m_count = 0u;   // comandos vivos, <= kInputHistory
    };

}
