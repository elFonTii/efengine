#pragma once
#include <efengine/core/Types.h>

// Los endianess son los algoritmos usados para guardar bytes en memoria.
// little-endian: el byte menos significativo en la direccion mas baja de memoria.
// big-endian: el byte mas significativo en la direccion mas baja de memoria.

// Numero en hex: 0x12345678
// Big-endian: [Byte 0] = 12 [Byte 1] = 34 [Byte 2] = 56 [Byte 3] = 78 (de izquierda a derecha, como leemos)
// Little-endian: [Byte 0] = 78 [Byte 1] = 56 [Byte 2] = 34 [Byte 3] = 12 (asi se maneja x86/x64 y ARM)
namespace efengine {
namespace serialization {

    // Índice "aca no hay nada" para los nodos y attachments. 
    // Se usa en el archivo para indicar que un nodo no tiene padre, o que un nodo no tiene mesh/light.
    inline constexpr u32 kInvalidIndex = 0xFFFFFFFFu;

    // Si al leer sale 0x04030201 el archivo es de otra endianness.
    inline constexpr u32 kEndianCheck = 0x01020304u;

    // Version del esquema. v2 agrega los escalares de emissive/normalStrength al
    // MaterialRecord y cambia el significado del primer f32 del chunk SCNE
    // (ambientFactor -> iblIntensity). v3 agrega doubleSided al MaterialRecord.
    // v4 agrega uvTiling/uvOffset al MaterialRecord.
    inline constexpr u32 kCurrentVersion      = 4u;

    // Version mas vieja que esta build todavia sabe leer. Se escribe SIEMPRE
    // kCurrentVersion: leer v1 es solo para no dejar ilegibles las escenas guardadas.
    inline constexpr u32 kMinSupportedVersion = 1u;

    inline constexpr u8 kMagic[4] = { 'E', 'F', 'E', '1' };

    // Arma un id de chunk de 4 caracteres. No se usan literales multi-caracter
    // ('STRT') porque su valor es implementation-defined en C++.
    constexpr u32 FourCC(char a, char b, char c, char d) {
        return static_cast<u32>(static_cast<u8>(a))
             | (static_cast<u32>(static_cast<u8>(b)) << 8)
             | (static_cast<u32>(static_cast<u8>(c)) << 16)
             | (static_cast<u32>(static_cast<u8>(d)) << 24);
    }

    // Que describe el archivo. El container es el mismo para todos.
    enum class ContentType : u32 {
        Scene = 1,
        // Mesh = 2, Material = 3 prox...
    };

    enum class ChunkId : u32 {
        Strings   = FourCC('S', 'T', 'R', 'T'),
        Settings  = FourCC('S', 'C', 'N', 'E'),
        Materials = FourCC('M', 'A', 'T', 'L'),
        Nodes     = FourCC('N', 'O', 'D', 'E'),
    };

    // Cantidad de chunks que emite una escena. Va en el header del archivo.
    inline constexpr u32 kSceneChunkCount = 4u;

    // --- Reglas de compatibilidad -----------------------------------------------
    // * Chunk nuevo                       -> NO sube kCurrentVersion
    //   (el reader saltea por byteSize lo que no conoce).
    // * Campo nuevo al final de un record -> SUBE kCurrentVersion.
    // * Cambio de significado de un campo -> SUBE kCurrentVersion.
    // Una version mayor a kCurrentVersion se rechaza (no se puede adivinar el futuro).
    // Una version entre kMinSupportedVersion y kCurrentVersion se lee con los campos
    // que esa version tenia: los Serialize ramifican con ar.Version(). El writer
    // siempre emite kCurrentVersion, asi que abrir y guardar migra el archivo.
    // ---------------------------------------------------------------------------

    class BinaryWriter;   // fwd
    class BinaryReader;   // fwd

    // Header: magic(4) + endianCheck(4) + version(4) + contentType(4) + chunkCount(4).
    void WriteFileHeader(BinaryWriter& w, ContentType type, u32 chunkCount);
    bool ReadFileHeader(BinaryReader& r, ContentType expected, u32& outChunkCount);

    // Framing: id(4) + byteSize(4) + payload padeado a 4.
    usize BeginChunk(BinaryWriter& w, ChunkId id);
    void  EndChunk(BinaryWriter& w, usize marker);

    // Lee el proximo encabezado de chunk. 'outEnd' es el offset absoluto donde termina:
    // pasalo a BinaryReader::EndSizedBlock para saltear lo que no leiste.
    bool NextChunk(BinaryReader& r, ChunkId& outId, usize& outEnd);

}
}
