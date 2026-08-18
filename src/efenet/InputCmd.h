#pragma once
#include <efenet/Types.h>

namespace efenet {

    namespace Button {
        inline constexpr u8 Forward = 1u << 0;
        inline constexpr u8 Back    = 1u << 1;
        inline constexpr u8 Left    = 1u << 2;
        inline constexpr u8 Right   = 1u << 3;
    }

    // Lo que el cliente manda cada tick. 9 bytes serializados.
    //
    // El cliente NUNCA manda su posicion: solo botones y mirada. El
    // teleport-hack queda imposible por construccion, no por validacion.
    //
    // El yaw viaja en el input (no solo en el estado) porque la mirada tiene
    // que ser instantanea: el cliente la decide y el servidor la copia tal cual.
    struct InputCmd {
        u32 seq     = 0u;    // secuencia monotona por cliente, arranca en 1
        f32 yaw     = 0.0f;  // radianes
        u8  buttons = 0u;    // bitmask de Button::

        template <class Ar>
        void Serialize(Ar& ar) {
            ar.Field(seq);
            ar.Field(yaw);
            ar.Field(buttons);
        }
    };

}
