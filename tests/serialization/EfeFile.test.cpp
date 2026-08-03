#include <doctest/doctest.h>
#include <efengine/serialization/EfeFile.h>
#include <efengine/serialization/BinaryWriter.h>
#include <efengine/serialization/BinaryReader.h>
#include <vector>

using namespace efengine;
using namespace efengine::serialization;

namespace {
    // Arma un archivo con 'chunkCount' chunks; cada uno lleva un u32 de carga.
    // Se usa como base valida que los tests despues corrompen a mano.
    std::vector<u8> makeFile(u32 version, u32 endianCheck, ContentType type,
                             const std::vector<std::pair<ChunkId, u32>>& chunks) {
        BinaryWriter w;
        // Header a mano (no via WriteFileHeader) para poder inyectar valores invalidos.
        w.Bytes(kMagic, 4u);
        u32 endian = endianCheck;
        u32 ver    = version;
        u32 content = static_cast<u32>(type);
        u32 count  = static_cast<u32>(chunks.size());
        w.Field(endian);
        w.Field(ver);
        w.Field(content);
        w.Field(count);

        for (const auto& c : chunks) {
            const usize marker = BeginChunk(w, c.first);
            u32 payload = c.second;
            w.Field(payload);
            EndChunk(w, marker);
        }
        return w.Take();
    }

    const ChunkId kUnknown = static_cast<ChunkId>(FourCC('X', 'X', 'X', 'X'));
}

TEST_CASE("EfeFile: header valido round-trip") {
    BinaryWriter w;
    WriteFileHeader(w, ContentType::Scene, 3u);

    BinaryReader r(w.Buffer());
    u32 chunkCount = 0u;
    CHECK(ReadFileHeader(r, ContentType::Scene, chunkCount));
    CHECK(r.Ok());
    CHECK(chunkCount == 3u);
    CHECK(r.Remaining() == 0u);
}

TEST_CASE("EfeFile: archivo con dos chunks se recorre completo") {
    const std::vector<u8> bytes = makeFile(
        kCurrentVersion, kEndianCheck, ContentType::Scene,
        { { ChunkId::Settings, 111u }, { ChunkId::Nodes, 222u } });

    BinaryReader r(bytes);
    u32 chunkCount = 0u;
    REQUIRE(ReadFileHeader(r, ContentType::Scene, chunkCount));
    REQUIRE(chunkCount == 2u);

    u32 vistos = 0u, sumaPayloads = 0u;
    for (u32 i = 0u; i < chunkCount; ++i) {
        ChunkId id = kUnknown;
        usize   end = 0u;
        REQUIRE(NextChunk(r, id, end));
        u32 payload = 0u;
        r.Field(payload);
        sumaPayloads += payload;
        r.EndSizedBlock(end);
        ++vistos;
    }

    CHECK(r.Ok());
    CHECK(vistos == 2u);
    CHECK(sumaPayloads == 333u);
    CHECK(r.Remaining() == 0u);
}

TEST_CASE("EfeFile: un chunk desconocido se saltea y el resto se lee igual") {
    // El chunk del medio es basura para este lector: tiene que saltearlo por tamano.
    const std::vector<u8> bytes = makeFile(
        kCurrentVersion, kEndianCheck, ContentType::Scene,
        { { ChunkId::Strings, 1u }, { kUnknown, 999u }, { ChunkId::Nodes, 7u } });

    BinaryReader r(bytes);
    u32 chunkCount = 0u;
    REQUIRE(ReadFileHeader(r, ContentType::Scene, chunkCount));
    REQUIRE(chunkCount == 3u);

    u32 leidoStrings = 0u, leidoNodes = 0u, salteados = 0u;
    for (u32 i = 0u; i < chunkCount; ++i) {
        ChunkId id = kUnknown;
        usize   end = 0u;
        REQUIRE(NextChunk(r, id, end));

        if (id == ChunkId::Strings)    r.Field(leidoStrings);
        else if (id == ChunkId::Nodes) r.Field(leidoNodes);
        else                           ++salteados;   // no se lee nada del desconocido

        r.EndSizedBlock(end);
    }

    CHECK(r.Ok());
    CHECK(salteados == 1u);
    CHECK(leidoStrings == 1u);
    CHECK(leidoNodes == 7u);        // el chunk de despues del desconocido se leyo bien
    CHECK(r.Remaining() == 0u);
}

TEST_CASE("EfeFile: magic invalido se rechaza") {
    std::vector<u8> bytes = makeFile(kCurrentVersion, kEndianCheck, ContentType::Scene,
                                     { { ChunkId::Nodes, 1u } });
    bytes[0] = 'Z';

    BinaryReader r(bytes);
    u32 chunkCount = 0u;
    CHECK_FALSE(ReadFileHeader(r, ContentType::Scene, chunkCount));
}

TEST_CASE("EfeFile: endianness invertida se rechaza") {
    const std::vector<u8> bytes = makeFile(kCurrentVersion, 0x04030201u, ContentType::Scene,
                                           { { ChunkId::Nodes, 1u } });

    BinaryReader r(bytes);
    u32 chunkCount = 0u;
    CHECK_FALSE(ReadFileHeader(r, ContentType::Scene, chunkCount));
}

TEST_CASE("EfeFile: version futura se rechaza") {
    const std::vector<u8> bytes = makeFile(kCurrentVersion + 1u, kEndianCheck, ContentType::Scene,
                                           { { ChunkId::Nodes, 1u } });

    BinaryReader r(bytes);
    u32 chunkCount = 0u;
    CHECK_FALSE(ReadFileHeader(r, ContentType::Scene, chunkCount));
}

TEST_CASE("EfeFile: contentType distinto del esperado se rechaza") {
    const std::vector<u8> bytes = makeFile(kCurrentVersion, kEndianCheck,
                                           static_cast<ContentType>(42u),
                                           { { ChunkId::Nodes, 1u } });

    BinaryReader r(bytes);
    u32 chunkCount = 0u;
    CHECK_FALSE(ReadFileHeader(r, ContentType::Scene, chunkCount));
}

TEST_CASE("EfeFile: header truncado se rechaza sin leer OOB") {
    const std::vector<u8> bytes = makeFile(kCurrentVersion, kEndianCheck, ContentType::Scene,
                                           { { ChunkId::Nodes, 1u } });

    BinaryReader r(bytes.data(), 10u);   // menos que los 20 bytes del header
    u32 chunkCount = 0u;
    CHECK_FALSE(ReadFileHeader(r, ContentType::Scene, chunkCount));
}

TEST_CASE("EfeFile: chunk que promete mas bytes de los que hay se rechaza") {
    std::vector<u8> bytes = makeFile(kCurrentVersion, kEndianCheck, ContentType::Scene,
                                     { { ChunkId::Nodes, 1u } });
    // El byteSize del chunk vive justo despues del header (20) y del id (4).
    const usize sizeOffset = 20u + 4u;
    REQUIRE(bytes.size() > sizeOffset + 4u);
    bytes[sizeOffset]     = 0xFFu;
    bytes[sizeOffset + 1] = 0xFFu;

    BinaryReader r(bytes);
    u32 chunkCount = 0u;
    REQUIRE(ReadFileHeader(r, ContentType::Scene, chunkCount));

    ChunkId id = kUnknown;
    usize   end = 0u;
    CHECK_FALSE(NextChunk(r, id, end));
}

TEST_CASE("EfeFile: un header v1 se acepta y el reader recuerda la version") {
    const std::vector<u8> bytes = makeFile(1u, kEndianCheck, ContentType::Scene,
                                           { { ChunkId::Nodes, 1u } });

    BinaryReader r(bytes);
    u32 chunkCount = 0u;
    CHECK(ReadFileHeader(r, ContentType::Scene, chunkCount));
    CHECK(r.Version() == 1u);
}

TEST_CASE("EfeFile: lo que escribe el writer se lee como la version actual") {
    BinaryWriter w;
    WriteFileHeader(w, ContentType::Scene, 1u);

    BinaryReader r(w.Buffer());
    u32 chunkCount = 0u;
    REQUIRE(ReadFileHeader(r, ContentType::Scene, chunkCount));
    CHECK(r.Version() == kCurrentVersion);
    CHECK(kCurrentVersion == 4u);
    CHECK(w.Version() == kCurrentVersion);
}

TEST_CASE("EfeFile: version 0 se rechaza") {
    const std::vector<u8> bytes = makeFile(0u, kEndianCheck, ContentType::Scene,
                                           { { ChunkId::Nodes, 1u } });

    BinaryReader r(bytes);
    u32 chunkCount = 0u;
    CHECK_FALSE(ReadFileHeader(r, ContentType::Scene, chunkCount));
}

TEST_CASE("EfeFile: FourCC es estable y distingue chunks") {
    CHECK(FourCC('S', 'T', 'R', 'T') == static_cast<u32>(ChunkId::Strings));
    CHECK(FourCC('S', 'C', 'N', 'E') == static_cast<u32>(ChunkId::Settings));
    CHECK(FourCC('M', 'A', 'T', 'L') == static_cast<u32>(ChunkId::Materials));
    CHECK(FourCC('N', 'O', 'D', 'E') == static_cast<u32>(ChunkId::Nodes));
    CHECK(FourCC('S', 'T', 'R', 'T') != FourCC('T', 'R', 'T', 'S'));
}
