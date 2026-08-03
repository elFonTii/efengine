#pragma once
#include <efengine/core/Types.h>

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    // -- Constantes del volumen ---------------------------------------------
    // El target de captura se aloca UNA vez al maximo y se usa el sub-rango del
    // frame: mover el slider de probes por frame nunca realoca nada.
    inline constexpr u32 kMaxProbesPerFrame = 32u;

    // Resolucion de cada cara de la captura. 6 caras de 16x16 = 1536 direcciones
    // por probe, contra los 64-256 rayos del paper. El costo escala con
    // geometria, no con rayos.
    inline constexpr u32 kProbeFaceSize = 16u;

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

    // Rango de probes a actualizar este frame. count siempre es min(perFrame, total).
    struct UpdateRange { u32 first = 0u; u32 count = 0u; u32 nextCursor = 0u; };

    // -- Funciones puras ----------------------------------------------------

    // Clampea counts a [1, kMaxProbesPerAxis] y spacing a un minimo positivo.
    // Los valores vienen de sliders de ImGui: son recuperables, se corrigen y se
    // loguea, no se asserta.
    DdgiGrid SanitizeGrid(const DdgiGrid& grid);

    u32 ProbeCount(const DdgiGrid& grid);

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

    // Round-robin contiguo con wrap. El shader hace (first + slot) % total.
    UpdateRange NextRange(u32 cursor, u32 perFrame, u32 total);

}
}
