#include <efenet/Protocol.h>

#include <efengine/serialization/BinaryWriter.h>
#include <efengine/serialization/BinaryReader.h>

#include <utility>

namespace efenet {

namespace {

    using efengine::serialization::BinaryReader;
    using efengine::serialization::BinaryWriter;

    // Todo Read* arranca igual: verificar que el primer byte sea el msgId que
    // esperabamos, y dejar el reader posicionado despues de el. Sin este
    // chequeo, leer un Welcome con ReadSnapshot devolveria basura que entra
    // comoda en el buffer.
    bool abrir(const u8* data, usize size, MsgId esperado, BinaryReader& out) {
        if (data == null || size == 0u) return false;
        if (data[0] != static_cast<u8>(esperado)) return false;
        out = BinaryReader(data + 1, size - 1u);
        return true;
    }

    void escribirId(BinaryWriter& w, MsgId id) {
        u8 byte = static_cast<u8>(id);
        w.Field(byte);
    }

}

    std::vector<u8> WriteWelcome(const WelcomeMsg& msg) {
        BinaryWriter w;
        escribirId(w, MsgId::Welcome);

        u16      version = msg.protocolVersion;
        PlayerId id      = msg.yourId;
        TickId   tick    = msg.serverTick;
        w.Field(version);
        w.Field(id);
        w.Field(tick);

        return w.Take();
    }

    std::vector<u8> WriteInput(const InputMsg& msg) {
        BinaryWriter w;
        escribirId(w, MsgId::Input);

        u8 count = static_cast<u8>(msg.commands.size());
        w.Field(count);
        for (InputCmd cmd : msg.commands) cmd.Serialize(w);

        return w.Take();
    }

    std::vector<u8> WriteSnapshot(const SnapshotMsg& msg) {
        BinaryWriter w;
        escribirId(w, MsgId::Snapshot);

        TickId tick  = msg.serverTick;
        u32    seq   = msg.lastInputSeq;
        u8     count = static_cast<u8>(msg.players.size());
        w.Field(tick);
        w.Field(seq);
        w.Field(count);
        for (PlayerState p : msg.players) p.Serialize(w);

        return w.Take();
    }

    bool PeekMsgId(const u8* data, usize size, MsgId& out) {
        if (data == null || size == 0u) return false;
        switch (data[0]) {
            case static_cast<u8>(MsgId::Welcome):  out = MsgId::Welcome;  return true;
            case static_cast<u8>(MsgId::Input):    out = MsgId::Input;    return true;
            case static_cast<u8>(MsgId::Snapshot): out = MsgId::Snapshot; return true;
            default: return false;
        }
    }

    bool ReadWelcome(const u8* data, usize size, WelcomeMsg& out) {
        BinaryReader r(null, 0u);
        if (!abrir(data, size, MsgId::Welcome, r)) return false;

        WelcomeMsg tmp;
        r.Field(tmp.protocolVersion);
        r.Field(tmp.yourId);
        r.Field(tmp.serverTick);
        if (!r.Ok()) return false;

        out = tmp;
        return true;
    }

    bool ReadInput(const u8* data, usize size, InputMsg& out) {
        BinaryReader r(null, 0u);
        if (!abrir(data, size, MsgId::Input, r)) return false;

        u8 count = 0u;
        r.Field(count);
        if (!r.Ok()) return false;

        // Validar en la frontera (principio 9): un cliente que manda mas de
        // kInputRedundancy esta fuera de contrato. El chequeo va ANTES de
        // reservar memoria o leer nada.
        if (count == 0u || count > kInputRedundancy) return false;

        InputMsg tmp;
        tmp.commands.resize(count);
        for (InputCmd& cmd : tmp.commands) cmd.Serialize(r);
        if (!r.Ok()) return false;

        out = std::move(tmp);
        return true;
    }

    bool ReadSnapshot(const u8* data, usize size, SnapshotMsg& out) {
        BinaryReader r(null, 0u);
        if (!abrir(data, size, MsgId::Snapshot, r)) return false;

        SnapshotMsg tmp;
        r.Field(tmp.serverTick);
        r.Field(tmp.lastInputSeq);

        u8 count = 0u;
        r.Field(count);
        if (!r.Ok()) return false;

        // count == 0 es valido: el servidor difunde aunque no haya nadie.
        if (count > kMaxPlayers) return false;

        tmp.players.resize(count);
        for (PlayerState& p : tmp.players) p.Serialize(r);
        if (!r.Ok()) return false;

        out = std::move(tmp);
        return true;
    }

}
