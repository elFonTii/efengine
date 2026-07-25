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

    // Version del esquema
    inline constexpr u32 kCurrentVersion = 1u;

    inline constexpr u8 kMagic[4] = { 'E', 'F', 'E', '1' };

}
}
