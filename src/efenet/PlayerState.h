#pragma once
#include <efenet/Types.h>

#include <glm/glm.hpp>

namespace efenet {

    // El estado authoritative de UN jugador. 20 bytes serializados.
    //
    // NO tiene velocidad a proposito: el movimiento del demo no tiene inercia,
    // asi que la velocidad sale del input y no del estado. Menos estado que
    // sincronizar es menos lugares donde cliente y servidor pueden divergir.
    // Para extrapolar, la velocidad se deriva de los dos ultimos snapshots.
    //
    // El yaw esta en RADIANES, como todo lo que cruza la capa de red: Step()
    // hace trigonometria y sin/cos toman radianes. La unica conversion a grados
    // vive en Avatars::Sync, al escribir el Transform del nodo.
    struct PlayerState {
        PlayerId  id       = kInvalidPlayer;
        glm::vec3 position { 0.0f };
        f32       yaw      = 0.0f;

        // Un solo metodo para leer y escribir: BinaryWriter y BinaryReader
        // exponen la misma API Field(). Campo a campo, sin padding de struct.
        template <class Ar>
        void Serialize(Ar& ar) {
            ar.Field(id);
            ar.Field(position);
            ar.Field(yaw);
        }
    };

}
