#pragma once

#include "EditorUI.h"

#include <efengine/core/Types.h>
#include <efengine/scene/NodeHandle.h>

namespace sandbox {

    // Indice del material "default" de la escena, creandolo la primera vez que
    // se lo pide. Devuelve SceneAssets::kInvalidIndex si no se pudo construir
    // (shader pbr que no compila, por ejemplo). El motor no conoce este
    // concepto: el default es un MaterialDef mas, guardado como cualquier otro.
    u32 EnsureDefaultMaterial(EditorContext& ctx);

    // Seccion "Malla" del Inspector: adjuntar un modelo del catalogo, quitarlo,
    // y elegir el material de cada submesh.
    void DrawMeshSection(EditorContext& ctx, efengine::scene::NodeHandle handle);

    // Panel "Materiales": lista de la escena + editor del material activo.
    // Editar un material pega en TODOS los nodos que lo usan: es de la escena,
    // no del nodo. Para variar uno solo existe "Duplicar".
    void DrawMaterialsPanel(EditorContext& ctx);

}
