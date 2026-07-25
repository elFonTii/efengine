#include "efengine/serialization/EfeFile.h"
#include <efengine/serialization/BinaryWriter.h>
#include <efengine/serialization/BinaryReader.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace serialization {

    void WriteFileHeader(BinaryWriter& w, ContentType type, u32 chunkCount) {
        w.Bytes(kMagic, 4u);
        u32 endian  = kEndianCheck;
        u32 version = kCurrentVersion;
        u32 content = static_cast<u32>(type);
        w.Field(endian);
        w.Field(version);
        w.Field(content);
        w.Field(chunkCount);
    }

    bool ReadFileHeader(BinaryReader& r, ContentType expected, u32& outChunkCount) {
        outChunkCount = 0u;

        u8 magic[4] = { 0u, 0u, 0u, 0u };
        r.Bytes(magic, 4u);
        // Se chequea Ok() si el buffer es más corto que el header, para no leer basura.
        if (!r.Ok()) {
            EF_LOG_ERROR("EfeFile: archivo truncado (no entra ni el header)");
            return false;
        }
        for (u32 i = 0u; i < 4u; ++i) {
            if (magic[i] != kMagic[i]) {
                EF_LOG_ERROR("EfeFile: magic invalido (no es un archivo .efe)");
                r.Fail();
                return false;
            }
        }

        u32 endian = 0u, version = 0u, content = 0u;
        r.Field(endian);
        r.Field(version);
        r.Field(content);
        r.Field(outChunkCount);
        if (!r.Ok()) {
            EF_LOG_ERROR("EfeFile: header truncado");
            outChunkCount = 0u;
            return false;
        }

        if (endian != kEndianCheck) {
            EF_LOG_ERROR("EfeFile: el archivo es de otra endianness (leyo 0x%08X)", endian);
            r.Fail(); outChunkCount = 0u; return false;
        }
        if (version != kCurrentVersion) {
            EF_LOG_ERROR("EfeFile: version %u no soportada (esta build lee %u)",
                         version, kCurrentVersion);
            r.Fail(); outChunkCount = 0u; return false;
        }
        if (content != static_cast<u32>(expected)) {
            EF_LOG_ERROR("EfeFile: contentType %u, se esperaba %u",
                         content, static_cast<u32>(expected));
            r.Fail(); outChunkCount = 0u; return false;
        }
        return true;
    }

    usize BeginChunk(BinaryWriter& w, ChunkId id) {
        u32 raw = static_cast<u32>(id);
        w.Field(raw);
        return w.BeginSizedBlock();
    }

    void EndChunk(BinaryWriter& w, usize marker) {
        w.EndSizedBlock(marker);
    }

    bool NextChunk(BinaryReader& r, ChunkId& outId, usize& outEnd) {
        u32 raw = 0u;
        r.Field(raw);
        if (!r.Ok()) return false;

        outEnd = r.BeginSizedBlock();
        if (!r.Ok()) {
            EF_LOG_ERROR("EfeFile: chunk con byteSize invalido");
            return false;
        }
        outId = static_cast<ChunkId>(raw);
        return true;
    }

}
}
