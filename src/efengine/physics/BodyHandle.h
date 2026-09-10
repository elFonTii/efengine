#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace physics {

    // Mismo esquema que scene::NodeHandle: el indice se reusa, la generacion
    // evita que un handle viejo apunte al inquilino nuevo.
    struct BodyHandle {
        u32 index      = 0;
        u32 generation = 0;   // 0 = handle nulo

        bool IsNull() const { return generation == 0; }
        bool operator==(const BodyHandle& other) const {
            return index == other.index && generation == other.generation;
        }
        bool operator!=(const BodyHandle& other) const { return !(*this == other); }
    };

}
}
