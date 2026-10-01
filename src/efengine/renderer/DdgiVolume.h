#pragma once
#include <efengine/core/Types.h>

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    // -- Constantes del volumen ---------------------------------------------
    // Filas del buffer de rayos: el tope de probes que un frame puede actualizar.
    inline constexpr u32 kMaxProbesPerFrame = 512u;

    // Tiles octaedricos, sin el borde. El de irradiancia es chico porque una
    // senal difusa no tiene alta frecuencia; el de distancia necesita mas
    // resolucion angular porque de el sale el test de Chebyshev.
    inline constexpr u32 kIrradianceTile = 8u;
    inline constexpr u32 kDistanceTile   = 16u;

    // Borde de 1 texel por lado. Existe porque el filtrado bilineal en el borde
    // de un tile octaedrico tiene que mezclar hacia el vecino ENVUELTO, no hacia
    // el tile de al lado.
    inline constexpr u32 kTileBorder = 1u;

    inline constexpr u32 kIrradianceTileBordered = kIrradianceTile + 2u * kTileBorder;  // 10
    inline constexpr u32 kDistanceTileBordered   = kDistanceTile   + 2u * kTileBorder;  // 18

    // 256 es el valor de RTXGI y el tamano del cache de rayos en shared memory de los
    // blends (kDdgiMaxRays en ddgi/update.glsl).
    inline constexpr u32 kMaxRaysPerProbe = 256u;
    inline constexpr u32 kMinRaysPerProbe = 64u;

    // local_size_x de ddgi/trace_voxel.comp.
    inline constexpr u32 kRayGroupSize = 64u;

    // Tope de probes. 32x32x32 = 32768 probes serian ~26 MB de atlas y 6*32768
    // draws de escena por barrido: absurdo, pero el clamp existe para que un
    // slider mal arrastrado no intente alocar gigabytes.
    inline constexpr i32 kMaxProbesPerAxis = 32;

    // -- Grilla -------------------------------------------------------------
    // Solo datos. Los defaults son para assets/scenes/interior.efe y se tunean
    // por ImGui; no estan serializados (eso es el ciclo de v4).
    struct DdgiGrid {
        glm::vec3  origin  { -6.0f, 0.2f, -6.0f };   // posicion mundo del probe (0,0,0)
        glm::vec3  spacing {  1.5f, 1.5f,  1.5f };   // metros entre probes
        glm::ivec3 counts  {  8, 4, 8 };             // probes por eje
    };

    // -- Funciones puras ----------------------------------------------------

    // Clampea counts a [1, kMaxProbesPerAxis] y spacing a un minimo positivo.
    // Los valores vienen de sliders de ImGui: son recuperables, se corrigen y se
    // loguea, no se asserta.
    DdgiGrid SanitizeGrid(const DdgiGrid& grid);

    u32 ProbeCount(const DdgiGrid& grid);

    // Bytes del SSBO de datos por probe: un vec4 (offset.xyz, fraccion de backfaces).
    usize ProbeDataBytes(const DdgiGrid& grid);

    // Un uvec4 por probe: edad, estado y dos libres.
    usize ProbeStateBytes(const DdgiGrid& grid);

    // Techo de las distancias del atlas de Chebyshev: 1,5 x la diagonal de una celda, como
    // RTXGI. El rayo es mas largo; esto solo acota lo que se integra.
    f32 DistanceClamp(const DdgiGrid& grid);

    // Espejo C++ de DdgiSphericalFibonacci (ddgi/update.glsl), para los tests.
    glm::vec3 FibonacciDirection(u32 i, u32 count);

    // Rotacion uniforme (Shoemake sobre un hash PCG), determinista por semilla.
    glm::mat3 RandomRayRotation(u32 seed);

    // 0 en las dos primeras escrituras despues de un reset (la primera puede venir de rayos
    // trazados desde la posicion vieja); despues promedio progresivo (n-1)/n con tope hMax.
    f32 EffectiveHysteresis(u32 age, f32 hMax);

    // index = x + countX * (y + countY * z), y su inversa.
    glm::ivec3 ProbeCoords(const DdgiGrid& grid, u32 index);
    u32        ProbeIndex(const DdgiGrid& grid, glm::ivec3 coords);

    glm::vec3 ProbeWorldPosition(const DdgiGrid& grid, u32 index);

    // Layout del atlas: columnas = countX * countY, filas = countZ. Asi
    // tileX = index % (countX*countY) y tileY = z, que es la bijeccion mas
    // barata que el shader puede evaluar.
    glm::ivec2 AtlasTileCount(const DdgiGrid& grid);
    glm::ivec2 AtlasTileCoords(const DdgiGrid& grid, u32 index);

    // Tamano en texels de cada atlas, tiles CON borde.
    glm::ivec2 IrradianceAtlasSize(const DdgiGrid& grid);
    glm::ivec2 DistanceAtlasSize(const DdgiGrid& grid);

}
}
