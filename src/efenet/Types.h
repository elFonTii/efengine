#pragma once
#include <efengine/core/Types.h>

namespace efenet {

    using PlayerId = u32;

    // Se llama TickId y no Tick porque Server tiene un metodo Tick(): el choque
    // entre el tipo y el metodo no compila limpio dentro de la clase.
    using TickId = u32;

    // Ningun jugador tiene id 0: los ids reales arrancan en 1. Asi un
    // PlayerState recien construido nunca se confunde con un jugador real.
    inline constexpr PlayerId kInvalidPlayer = 0u;

    // SIMULACION
    inline constexpr u32 kTickRate = 60u;
    inline constexpr f32 kTickDt   = 1.0f / static_cast<f32>(kTickRate);

    // ENVIO. El servidor simula a 60 Hz pero difunde a 20: mandar un snapshot
    // por tick triplicaria el bandwidth sin que se note en pantalla, porque la
    // interpolacion ya suaviza lo que llega.
    inline constexpr u32 kSnapshotRate     = 20u;
    inline constexpr u32 kTicksPerSnapshot = kTickRate / kSnapshotRate; // 3

    inline constexpr u32 kMaxPlayers = 8u;

    // Cada paquete de input lleva los ultimos 3 comandos, no solo el nuevo. Si
    // se pierde un paquete, el siguiente ya trae el que falto: recuperacion de
    // perdida sin acks ni retransmision, por 18 bytes.
    inline constexpr u32 kInputRedundancy = 3u;

    // Historial de inputs sin confirmar del cliente: ~2 s a 60 Hz. Alcanza para
    // cualquier RTT jugable.
    inline constexpr u32 kInputHistory = 128u;

    // COMPENSACION DE LATENCIA
    // Los remotos se dibujan 100 ms en el pasado. Con snapshots cada 50 ms eso
    // deja siempre dos disponibles para interpolar, y aguanta perder uno.
    inline constexpr f32 kInterpDelay = 0.100f;

    // Tope de extrapolacion. Sin tope, un jugador que se desconecta sale
    // caminando al infinito.
    inline constexpr f32 kMaxExtrapolation = 0.250f;

    // MUNDO
    inline constexpr f32 kMoveSpeed        = 6.0f;   // unidades/segundo
    inline constexpr f32 kPlayAreaHalfSize = 20.0f;  // clamp en X y Z

    // RED
    inline constexpr u16 kPort            = 7777u;
    inline constexpr u16 kProtocolVersion = 1u;

}
