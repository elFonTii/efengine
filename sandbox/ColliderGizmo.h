#pragma once

#include <efengine/core/Types.h>

#include <imgui.h>   // ImGuiID

namespace sandbox {

    struct EditorContext;

    // Dibuja el wireframe de cada collider de la escena, proyectado con
    // ImDrawList sobre la imagen final. Primitivas con su forma real; el
    // collider Mesh, como la AABB de su modelo, porque proyectar cada triangulo
    // a mano seria carisimo e ilegible.
    //
    // dockId 0 es valido: significa "no hay dockspace" y el rect cae al area de
    // trabajo del viewport. Una arista con algun extremo detras de la camara no
    // se dibuja.
    void DrawColliderGizmos(EditorContext& ctx, ImGuiID dockId);

}
