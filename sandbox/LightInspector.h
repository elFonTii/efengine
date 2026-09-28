#pragma once
#include <efengine/scene/NodeHandle.h>

namespace sandbox {

    struct EditorContext;

    // La seccion "Luz" del inspector: agregar, editar y quitar la luz del nodo.
    void DrawLightSection(EditorContext& ctx, efengine::scene::NodeHandle handle);

}
