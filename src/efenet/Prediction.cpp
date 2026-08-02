#include <efenet/Prediction.h>
#include <efenet/Simulation.h>

namespace efenet {

namespace {
    constexpr usize kCap = static_cast<usize>(kInputHistory);
}

    void Prediction::Push(const InputCmd& cmd) {
        m_ring[m_head] = cmd;
        m_head = (m_head + 1u) % kCap;
        if (m_count < kCap) ++m_count;
    }

    void Prediction::LastN(u32 n, std::vector<InputCmd>& out) const {
        out.clear();

        usize cuantos = static_cast<usize>(n);
        if (cuantos > m_count) cuantos = m_count;
        if (cuantos == 0u) return;

        // Indice del primero a copiar, contando hacia atras desde m_head. El
        // + kCap evita tomar el modulo de un numero negativo.
        usize idx = (m_head + kCap - cuantos) % kCap;
        for (usize i = 0u; i < cuantos; ++i) {
            out.push_back(m_ring[idx]);
            idx = (idx + 1u) % kCap;
        }
    }

    void Prediction::DiscardUpTo(u32 seq) {
        // Del mas viejo al mas nuevo, contando cuantos sobran. Los seq son
        // monotonos: en cuanto uno es mayor, los siguientes tambien.
        usize idx = (m_head + kCap - m_count) % kCap;
        usize aDescartar = 0u;
        for (usize i = 0u; i < m_count; ++i) {
            if (m_ring[idx].seq > seq) break;
            ++aDescartar;
            idx = (idx + 1u) % kCap;
        }
        m_count -= aDescartar;
    }

    void Prediction::Reconcile(PlayerState& state, const PlayerState& authoritative,
                               u32 lastProcessedSeq) {
        // 1. La verdad del servidor, sin discusion.
        state = authoritative;

        // 2. Lo que el servidor ya proceso esta reflejado ahi: fuera del historial.
        DiscardUpTo(lastProcessedSeq);

        // 3. Re-aplicar lo que el servidor todavia no vio, con el MISMO Step().
        usize idx = (m_head + kCap - m_count) % kCap;
        for (usize i = 0u; i < m_count; ++i) {
            Step(state, m_ring[idx], kTickDt);
            idx = (idx + 1u) % kCap;
        }
    }

    void Prediction::Clear() {
        m_head  = 0u;
        m_count = 0u;
    }

}
