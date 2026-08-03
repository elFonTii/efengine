// Types.h
//  Aliases de tipos de efecom. Espejo de los aliases del sistema (u32, f32...)
//  pero dentro del namespace efecom: el RHI no puede depender de efengine,
//  así que no puede incluir <efengine/core/Types.h>.
#pragma once
#include <cstddef>

namespace efecom {

    using u8  = unsigned char;
    using u16 = unsigned short;
    using u32 = unsigned int;
    using u64 = unsigned long long;

    using i8  = signed char;
    using i16 = short;
    using i32 = int;
    using i64 = long long;

    using f32 = float;
    using f64 = double;

    using usize = decltype(sizeof(0));

}
