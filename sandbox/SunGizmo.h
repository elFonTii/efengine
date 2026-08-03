#pragma once

#include <efengine/core/Types.h>

#include <glm/glm.hpp>

#include <imgui.h>   // ImGuiID

namespace sandbox {

    struct EditorContext;

    // Dibuja una flecha world-space desde el lado del sol hasta el centro de la
    // escena, proyectada con ImDrawList sobre la imagen final.
    //
    // dockId 0 es valido: significa "no hay dockspace", y el rect cae al area de
    // trabajo del viewport. No dibuja nada si algun extremo de la flecha queda
    // detras de la camara.
    void DrawSunGizmo(EditorContext& ctx, ImGuiID dockId);

    // Angulos legibles de la direccion de la luz, para el overlay de stats.
    // azimut: rumbo hacia el que VIAJA la luz, 0 = hacia -Z, positivo hacia +X.
    // elevacion: altura del SOL sobre el horizonte, positiva cuando esta arriba.
    void SunAngles(const glm::vec3& dir, f32& azimutDeg, f32& elevacionDeg);

}
