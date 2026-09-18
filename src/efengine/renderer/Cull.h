#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Bounds.h>

#include <vector>

namespace efengine {
namespace renderer {

    // Una submalla ubicada en el mundo. La unidad del culling es la SUBMALLA y
    // no el RenderItem porque un .fbx importado es un solo item con miles de
    // submallas adentro: cullear por item descarta cero.
    struct MeshSpan {
        u32  item   = 0u;   // indice en SceneGraph::Renderables()
        u32  mesh   = 0u;   // indice en item.model->meshes()
        AABB bounds {};     // en espacio de mundo
    };

    // Inclusivo en el contacto: geometria que apoya justo sobre el plano del
    // volumen cuenta como visible. Una pared que deja de bloquear luz es peor
    // que una submalla de mas.
    bool Overlaps(const AABB& a, const AABB& b);

    // Escribe en 'out' los indices de 'spans' que solapan con 'volume',
    // preservando el orden. Limpia 'out' sin liberar su capacidad: el llamador
    // lo tiene como miembro y lo reusa entre frames.
    void CullAabb(const std::vector<MeshSpan>& spans, const AABB& volume,
                  std::vector<u32>& out);

}
}
