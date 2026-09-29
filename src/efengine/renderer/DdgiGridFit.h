#pragma once
#include <efengine/renderer/Bounds.h>
#include <efengine/renderer/DdgiVolume.h>

namespace efengine {
namespace renderer {

    // Encaja origin/spacing a la caja con un 10% de margen hacia adentro: un probe
    // dentro de una pared contamina a sus vecinos por el peso trilineal. Conserva
    // counts. Devuelve false, sin tocar nada, si la caja es invalida.
    bool FitDdgiGridToBounds(DdgiGrid& grid, const AABB& bounds);

}
}
