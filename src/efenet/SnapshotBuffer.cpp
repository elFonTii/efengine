#include <efenet/SnapshotBuffer.h>

#include <cmath>

namespace efenet {

    f32 LerpAngle(f32 a, f32 b, f32 t) {
        // Llevar la diferencia al rango [-PI, PI]: ese es el camino corto.
        f32 delta = b - a;
        while (delta >  PI) delta -= TAU;
        while (delta < -PI) delta += TAU;
        return a + delta * t;
    }

    const SnapshotBuffer::Entry& SnapshotBuffer::at(usize i) const {
        const usize base = (m_head + kCapacity - m_count) % kCapacity;
        return m_ring[(base + i) % kCapacity];
    }

    void SnapshotBuffer::Push(f32 serverTime, const PlayerState& state) {
        // Fuera de orden: se descarta. No es un error, es UDP.
        if (m_count > 0u && serverTime <= NewestTime()) return;

        m_ring[m_head].time  = serverTime;
        m_ring[m_head].state = state;
        m_head = (m_head + 1u) % kCapacity;
        if (m_count < kCapacity) ++m_count;
    }

    f32 SnapshotBuffer::NewestTime() const {
        if (m_count == 0u) return 0.0f;
        return at(m_count - 1u).time;
    }

    bool SnapshotBuffer::Sample(f32 renderTime, bool allowExtrapolation, PlayerState& out) const {
        if (m_count == 0u) return false;

        if (m_count == 1u) {
            out = at(0u).state;
            return true;
        }

        const Entry& masViejo = at(0u);
        const Entry& ultimo   = at(m_count - 1u);

        // Antes de lo que tenemos: congelar en el mas viejo. Extrapolar hacia
        // atras no aporta nada y se ve peor.
        if (renderTime <= masViejo.time) {
            out = masViejo.state;
            return true;
        }

        // EXTRAPOLACION: se paso del ultimo snapshot.
        if (renderTime >= ultimo.time) {
            out = ultimo.state;
            if (!allowExtrapolation) return true;

            const Entry& anterior = at(m_count - 2u);
            const f32 intervalo = ultimo.time - anterior.time;
            if (intervalo <= EPSILON) return true;

            // La velocidad se DERIVA de los dos ultimos: por eso el snapshot no
            // necesita mandarla.
            const glm::vec3 vel = (ultimo.state.position - anterior.state.position) / intervalo;

            f32 avance = renderTime - ultimo.time;
            if (avance > kMaxExtrapolation) avance = kMaxExtrapolation;  // congela

            out.position = ultimo.state.position + vel * avance;
            return true;
        }

        // INTERPOLACION: buscar los dos que rodean renderTime.
        for (usize i = 0u; i + 1u < m_count; ++i) {
            const Entry& a = at(i);
            const Entry& b = at(i + 1u);
            if (renderTime < a.time || renderTime > b.time) continue;

            const f32 intervalo = b.time - a.time;
            const f32 t = (intervalo > EPSILON) ? (renderTime - a.time) / intervalo : 0.0f;

            out          = b.state;   // id y campos no interpolables
            out.position = a.state.position + (b.state.position - a.state.position) * t;
            out.yaw      = LerpAngle(a.state.yaw, b.state.yaw, t);
            return true;
        }

        // Inalcanzable: los casos de arriba cubren todo el rango. Por las dudas,
        // el ultimo conocido antes que un valor sin inicializar.
        out = ultimo.state;
        return true;
    }

    void SnapshotBuffer::Clear() {
        m_head  = 0u;
        m_count = 0u;
    }

}
