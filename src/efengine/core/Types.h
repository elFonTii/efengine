#pragma once
#include <cstddef>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using i8 = signed char;
using i16 = short;
using i32 = int;
using i64 = long long;

using f32 = float;
using f64 = double;

using b32 = u32; // Usamos u32 para booleanos por simplicidad y algunas plataformas lo prefieren. (BOOLEANO NO EXISTE EN C++)
using b8 = u8;

using usize = decltype(sizeof(0));

constexpr nullptr_t null = nullptr;

inline constexpr f32 PI = 3.14159265358979323846f;
inline constexpr f32 TAU = 6.28318530717958647692f;
inline constexpr f32 EPSILON = 1e-6f;
inline constexpr f32 RAD_TO_DEG = 180.0f / PI;
inline constexpr f32 DEG_TO_RAD = PI / 180.0f;