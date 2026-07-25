#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace scene {
    struct NodeHandle {
        u32 index = 0; // indice
        u32 generation = 0; // quien es el inquilino, un indice puede ser reutilizado, pero el inquilino es unico (como sql keys)

        bool IsNull() const { return generation == 0; }
        bool operator==(const NodeHandle& other) const {
            return index == other.index && generation == other.generation;
        }
        bool operator!=(const NodeHandle& other) const { return !(*this == other); }
    };
}
}