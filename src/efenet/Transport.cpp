#include <efenet/Transport.h>

#include <efengine/core/Log.h>

#include <enet/enet.h>

#include <utility>

namespace efenet {

namespace {

    // enet_initialize/deinitialize son globales del proceso y hay que llamarlas
    // una sola vez. Este guard las cuenta: el primer Transport inicializa, el
    // ultimo en morir finaliza. RAII, sin init()/shutdown() manuales
    // (principio 10).
    struct EnetGuard {
        static u32 refs;

        static bool Acquire() {
            if (refs == 0u) {
                if (enet_initialize() != 0) {
                    EF_LOG_ERROR("efenet: enet_initialize fallo");
                    return false;
                }
            }
            ++refs;
            return true;
        }

        static void Release() {
            if (refs == 0u) return;
            --refs;
            if (refs == 0u) enet_deinitialize();
        }
    };

    u32 EnetGuard::refs = 0u;

    ENetHost* host(void* p) { return static_cast<ENetHost*>(p); }

    // Dos canales: 0 para lo confiable (Welcome), 1 para lo no confiable
    // (Input, Snapshot). ENet garantiza orden por canal, y separarlos evita que
    // el handshake compita con el trafico de cada tick.
    constexpr enet_uint8 kChannelReliable   = 0u;
    constexpr enet_uint8 kChannelUnreliable = 1u;
    constexpr usize      kChannelCount      = 2u;

}

    Transport::Transport(u16 listenPort, u32 maxPeers)
        : m_maxPeers(maxPeers) {
        if (!EnetGuard::Acquire()) return;

        ENetAddress address {};
        ENetAddress* bind = null;
        if (listenPort != 0u) {
            address.host = ENET_HOST_ANY;
            address.port = listenPort;
            bind = &address;
        }

        m_host = enet_host_create(bind, maxPeers, kChannelCount, 0, 0);
        if (m_host == null) {
            EF_LOG_ERROR("efenet: enet_host_create fallo (puerto %u ocupado?)",
                         static_cast<u32>(listenPort));
            EnetGuard::Release();
            return;
        }
    }

    Transport::~Transport() {
        if (m_host != null) {
            enet_host_destroy(host(m_host));
            m_host = null;
            EnetGuard::Release();
        }
    }

    Transport::Transport(Transport&& other) noexcept
        : m_host(std::exchange(other.m_host, null)),
          m_serverPeer(std::exchange(other.m_serverPeer, null)),
          m_maxPeers(std::exchange(other.m_maxPeers, 0u)),
          m_scratch(std::move(other.m_scratch)) {
        other.m_scratch.clear();   // el origen queda vacio y valido
    }

    Transport& Transport::operator=(Transport&& other) noexcept {
        if (this != &other) {
            if (m_host != null) {
                enet_host_destroy(host(m_host));
                EnetGuard::Release();
            }
            m_host       = std::exchange(other.m_host, null);
            m_serverPeer = std::exchange(other.m_serverPeer, null);
            m_maxPeers   = std::exchange(other.m_maxPeers, 0u);
            m_scratch    = std::move(other.m_scratch);
            other.m_scratch.clear();
        }
        return *this;
    }

    bool Transport::Connect(const char* addressText, u16 port) {
        if (m_host == null || addressText == null) return false;

        ENetAddress address {};
        if (enet_address_set_host(&address, addressText) != 0) {
            EF_LOG_ERROR("efenet: no se pudo resolver '%s'", addressText);
            return false;
        }
        address.port = port;

        ENetPeer* peer = enet_host_connect(host(m_host), &address, kChannelCount, 0);
        if (peer == null) {
            EF_LOG_ERROR("efenet: no hay peers libres para conectar");
            return false;
        }

        m_serverPeer = peer;
        return true;
    }

    bool Transport::Poll(Event& out) {
        out = Event {};
        if (m_host == null) return false;

        ENetEvent ev {};
        // timeout 0: no bloquea nunca. El loop decide cuando volver.
        if (enet_host_service(host(m_host), &ev, 0) <= 0) return false;

        // El indice de peer sale de la aritmetica de punteros sobre el array de
        // ENet: es estable mientras viva el host.
        const u32 index = (ev.peer != null)
            ? static_cast<u32>(ev.peer - host(m_host)->peers)
            : 0u;
        out.peer = index;

        switch (ev.type) {
            case ENET_EVENT_TYPE_CONNECT:
                out.type = EventType::Connect;
                return true;

            case ENET_EVENT_TYPE_DISCONNECT:
                out.type = EventType::Disconnect;
                if (ev.peer == m_serverPeer) m_serverPeer = null;
                return true;

            case ENET_EVENT_TYPE_RECEIVE: {
                out.type = EventType::Receive;

                // Copiar ANTES de destruir. Devolver un puntero al packet de
                // ENet y despues destruirlo seria un use-after-free: el
                // llamador leeria memoria liberada.
                //
                // El scratch es miembro, asi que se reusa entre llamadas sin
                // realocar. De ahi sale el contrato documentado en Event::data:
                // valido hasta el proximo Poll(), porque el siguiente paquete
                // lo pisa.
                const u8* bytes = static_cast<const u8*>(ev.packet->data);
                m_scratch.assign(bytes, bytes + ev.packet->dataLength);
                enet_packet_destroy(ev.packet);

                out.data = m_scratch.data();
                out.size = m_scratch.size();
                return true;
            }

            default:
                out.type = EventType::None;
                return true;
        }
    }

    void Transport::Send(u32 peer, const u8* data, usize size, bool reliable) {
        if (m_host == null || data == null || size == 0u) return;
        if (peer >= m_maxPeers) return;

        ENetPeer* target = &host(m_host)->peers[peer];
        if (target->state != ENET_PEER_STATE_CONNECTED) return;

        const enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE
                                           : ENET_PACKET_FLAG_UNSEQUENCED;
        ENetPacket* packet = enet_packet_create(data, size, flags);
        if (packet == null) return;

        enet_peer_send(target, reliable ? kChannelReliable : kChannelUnreliable, packet);
    }

    void Transport::Broadcast(const u8* data, usize size, bool reliable) {
        if (m_host == null || data == null || size == 0u) return;

        const enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE
                                           : ENET_PACKET_FLAG_UNSEQUENCED;
        ENetPacket* packet = enet_packet_create(data, size, flags);
        if (packet == null) return;

        enet_host_broadcast(host(m_host), reliable ? kChannelReliable : kChannelUnreliable, packet);
    }

    void Transport::Disconnect(u32 peer) {
        if (m_host == null || peer >= m_maxPeers) return;
        enet_peer_disconnect(&host(m_host)->peers[peer], 0);
    }

    u32 Transport::RttMs(u32 peer) const {
        if (m_host == null || peer >= m_maxPeers) return 0u;
        const ENetPeer& p = host(m_host)->peers[peer];
        if (p.state != ENET_PEER_STATE_CONNECTED) return 0u;
        return static_cast<u32>(p.roundTripTime);
    }

}
