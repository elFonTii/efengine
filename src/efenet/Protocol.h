#pragma once
#include <efenet/PlayerState.h>
#include <efenet/InputCmd.h>

#include <vector>

namespace efenet {

    // Tres mensajes, y alcanza.
    //
    // No hay Hello: ENet ya avisa la conexion del peer con su propio evento.
    // No hay join/leave: la membresia es IMPLICITA, quien esta en el array del
    // snapshot esta en la partida, y el cliente borra a los que no aparecieron.
    enum class MsgId : u8 {
        Welcome  = 1u,   // S->C  confiable, al conectar
        Input    = 2u,   // C->S  no confiable, 60 Hz
        Snapshot = 3u,   // S->C  no confiable, 20 Hz
    };

    struct WelcomeMsg {
        u16      protocolVersion = kProtocolVersion;
        PlayerId yourId          = kInvalidPlayer;
        TickId   serverTick      = 0u;
    };

    struct InputMsg {
        // 1..kInputRedundancy comandos, del mas viejo al mas nuevo.
        std::vector<InputCmd> commands;
    };

    struct SnapshotMsg {
        TickId serverTick = 0u;

        // El ultimo input DE ESTE DESTINATARIO que el servidor proceso. Es lo
        // que ancla la reconciliacion, y es distinto para cada cliente: por eso
        // el snapshot se serializa una vez por cliente, no una sola vez para
        // todos.
        u32 lastInputSeq = 0u;

        // 0..kMaxPlayers.
        std::vector<PlayerState> players;
    };

    // ESCRITURA. Cada buffer arranca con u8 msgId.
    std::vector<u8> WriteWelcome (const WelcomeMsg& msg);
    std::vector<u8> WriteInput   (const InputMsg& msg);
    std::vector<u8> WriteSnapshot(const SnapshotMsg& msg);

    // Lee solo el msgId, sin consumir el cuerpo. false si el buffer esta vacio
    // o el id no es uno de los tres conocidos.
    bool PeekMsgId(const u8* data, usize size, MsgId& out);

    // LECTURA. Devuelven false si el buffer esta truncado, corrupto, miente en
    // un count, o trae otro mensaje. Con false, 'out' queda sin tocar.
    bool ReadWelcome (const u8* data, usize size, WelcomeMsg& out);
    bool ReadInput   (const u8* data, usize size, InputMsg& out);
    bool ReadSnapshot(const u8* data, usize size, SnapshotMsg& out);

}
