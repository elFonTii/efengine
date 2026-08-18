#include <efenet/NetSim.h>

#include <utility>

namespace efenet {

    bool NetSim::Active() const {
        return settings.latencyMs > 0.0f
            || settings.jitterMs  > 0.0f
            || settings.lossPct   > 0.0f;
    }

    bool NetSim::shouldDrop(bool reliable) {
        if (reliable) return false;              // ver el comentario del header
        if (settings.lossPct <= 0.0f) return false;

        std::uniform_real_distribution<f32> dado(0.0f, 100.0f);
        return dado(m_rng) < settings.lossPct;
    }

    f32 NetSim::deliveryTime(f32 now) {
        f32 retardoMs = settings.latencyMs;
        if (settings.jitterMs > 0.0f) {
            std::uniform_real_distribution<f32> jitter(0.0f, settings.jitterMs);
            retardoMs += jitter(m_rng);
        }
        return now + retardoMs * 0.001f;
    }

    void NetSim::PushOutbound(f32 now, const u8* data, usize size, bool reliable) {
        if (data == null || size == 0u) return;
        if (shouldDrop(reliable)) { ++m_dropped; return; }

        Packet p;
        p.deliverAt = deliveryTime(now);
        p.reliable  = reliable;
        p.data.assign(data, data + size);
        m_outbound.push_back(std::move(p));
    }

    void NetSim::PushInbound(f32 now, u32 peer, const u8* data, usize size) {
        if (data == null || size == 0u) return;
        if (shouldDrop(false)) { ++m_dropped; return; }

        Packet p;
        p.deliverAt = deliveryTime(now);
        p.peer      = peer;
        p.data.assign(data, data + size);
        m_inbound.push_back(std::move(p));
    }

    bool NetSim::PopOutbound(f32 now, std::vector<u8>& data, bool& reliable) {
        // Con jitter el frente puede no ser el mas vencido: se busca el primero
        // que ya cumplio. Entregar fuera de orden es correcto, es lo que hace
        // una red real.
        for (auto it = m_outbound.begin(); it != m_outbound.end(); ++it) {
            if (it->deliverAt > now) continue;
            data     = std::move(it->data);
            reliable = it->reliable;
            m_outbound.erase(it);
            return true;
        }
        return false;
    }

    bool NetSim::PopInbound(f32 now, u32& peer, std::vector<u8>& data) {
        for (auto it = m_inbound.begin(); it != m_inbound.end(); ++it) {
            if (it->deliverAt > now) continue;
            data = std::move(it->data);
            peer = it->peer;
            m_inbound.erase(it);
            return true;
        }
        return false;
    }

    void NetSim::Clear() {
        m_outbound.clear();
        m_inbound.clear();
    }

}
