#pragma once
#include <efengine/core/Types.h>

namespace efenet {

    // Version del protocolo de red. Sube con CUALQUIER cambio en la forma de un
    // paquete. El cliente que recibe un Welcome con otra version se desconecta:
    // dos builds desincronizados fallan ruidosamente en vez de comportarse raro.
    u16 ProtocolVersion();

}
