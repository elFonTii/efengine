#pragma once
#include <efenet/Types.h>

#include <vector>

namespace efenet {

    enum class EventType {
        None,
        Connect,
        Disconnect,
        Receive,
    };

    struct Event {
        EventType type = EventType::None;
        u32       peer = 0u;      // indice estable de peer, 0..maxPeers-1

        // Solo valido en Receive, y SOLO hasta el proximo Poll(): apunta al
        // buffer scratch del Transport, que el siguiente paquete pisa. Quien
        // necesite conservarlo, que lo copie.
        const u8* data = null;
        usize     size = 0u;
    };

    // Envoltorio RAII sobre ENet. Posee el host y el ciclo de vida global de la
    // libreria. No sabe nada del protocolo: mueve bytes.
    //
    // Handle-owner segun el principio 2, asi que RO5 completo: destructor,
    // copia borrada, move noexcept.
    class Transport {
        public:
            // Servidor: listenPort = kPort. Cliente: listenPort = 0, y despues
            // Connect(). Falla silenciosa: chequear Ok() despues de construir.
            Transport(u16 listenPort, u32 maxPeers);
            ~Transport();

            Transport(const Transport&)            = delete;
            Transport& operator=(const Transport&) = delete;
            Transport(Transport&& other) noexcept;
            Transport& operator=(Transport&& other) noexcept;

            bool Ok() const { return m_host != null; }

            // Arranca la conexion. No bloquea: el resultado llega como un evento
            // Connect en un Poll() posterior.
            bool Connect(const char* host, u16 port);

            // Drena un evento por llamada. false cuando no queda ninguno en esta
            // vuelta; el patron es un while(Poll(ev)).
            bool Poll(Event& out);

            void Send(u32 peer, const u8* data, usize size, bool reliable);
            void Broadcast(const u8* data, usize size, bool reliable);
            void Disconnect(u32 peer);

            // 0 si el peer no esta conectado.
            u32 RttMs(u32 peer) const;

            u32 MaxPeers() const { return m_maxPeers; }

        private:
            // void* para no filtrar los headers de ENet a todo el que incluya
            // este archivo. El .cpp los castea.
            void* m_host = null;

            // Peer al que nos conectamos en modo cliente. null en el servidor.
            void* m_serverPeer = null;

            u32 m_maxPeers = 0u;

            // Copia del ultimo paquete recibido. Existe porque ENet destruye su
            // packet dentro de Poll(): devolver un puntero al packet y despues
            // destruirlo seria un use-after-free.
            std::vector<u8> m_scratch;
    };

}
