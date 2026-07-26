#pragma once

#include <efengine/core/Types.h>

namespace efengine {
namespace platform {
    enum class Key : i32 {
        // ASCII
        Escape  = 256,
        Space   = 32,
        Tab     = 258,

        Down    = 264,
        Up      = 265,
        Left    = 263,
        Right   = 262,
        
        W       = 87,
        A       = 65,
        S       = 83,
        D       = 68,
        Q       = 81,
        E       = 69,
        F       = 70,

        LeftShift = 340,
        LeftAlt   = 342,
    };

    enum class MouseButton : i32 {
        Left        = 0,
        Right       = 1,
        Middle      = 2,
    };

    enum class InputAction : i32 {
        Release     = 0,
        Press       = 1,
        Repeat      = 2,
    };
}
}