// Types.h
//  Aliases de tipos para estandarizar u32, f32, etc...
#pragma once
#include <cstddef>

// Tipos enteros standard.
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using i8 = signed char;
using i16 = short;
using i32 = int;
using i64 = long long;

// Tipos de punto flotante standard.
using f32 = float;
using f64 = double;

// Booleanos
using b32 = u32; // Usamos u32 para booleanos por simplicidad y algunas plataformas lo prefieren. (BOOLEANO NO EXISTE EN C++)
using b8 = u8; // Booleano de 8 bits, no entiendo para qué.


// Tipo tamaño, para cosas como ancho/alto de pantalla, etc.
using usize = decltype(sizeof(0));

// Null pointers
constexpr nullptr_t null = nullptr; // Alias para nullptr, por si queremos usar "null" en lugar de "nullptr" en el código.

// Otros tipos útiles
inline constexpr f32 PI = 3.14159265358979323846f; // Constante de pi
inline constexpr f32 TAU = 6.28318530717958647692f; // Constante de tau (2 * pi)
inline constexpr f32 EPSILON = 1e-6f; // para comparaciones de flotantes.
inline constexpr f32 RAD_TO_DEG = 180.0f / PI; // rad to deg y viceversa
inline constexpr f32 DEG_TO_RAD = PI / 180.0f; 