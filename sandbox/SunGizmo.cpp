#include "SunGizmo.h"

#include "EditorUI.h"

#include <efengine/application/Application.h>
#include <efengine/math/Math.h>
#include <efengine/renderer/Bounds.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/SceneGraph.h>

#include <imgui.h>
#include <imgui_internal.h>   // DockBuilderGetCentralNode

#include <cmath>
#include <optional>

namespace sandbox {

using namespace efengine;

void SunAngles(const glm::vec3& dir, f32& azimutDeg, f32& elevacionDeg) {
    const f32 largo = glm::length(dir);
    if (largo < 1e-6f) { azimutDeg = 0.0f; elevacionDeg = 0.0f; return; }

    const glm::vec3 d = dir / largo;
    azimutDeg = glm::degrees(std::atan2(d.x, -d.z));
    // dir apunta DEL sol A la escena, asi que un sol alto tiene d.y negativo.
    elevacionDeg = glm::degrees(std::asin(glm::clamp(-d.y, -1.0f, 1.0f)));
}

void DrawSunGizmo(EditorContext& ctx, ImGuiID dockId) {
    const renderer::AABB& b = ctx.scene.WorldBounds();
    const glm::vec3 ancla = b.Valid() ? b.Center() : glm::vec3(0.0f);
    const f32       radio = b.Valid() ? glm::max(b.Radius(), 1.0f) : 10.0f;

    const glm::vec3 dir = ctx.scene.Sun().direction;
    if (glm::length(dir) < 1e-6f) return;

    // La punta LLEGA al centro de la escena y la cola queda del lado del sol: se
    // lee como "el sol viene de alla".
    const glm::vec3 cola = ancla - glm::normalize(dir) * (0.6f * radio);

    // El rect es el nodo central del dockspace (el agujero passthrough) para no
    // pintar encima de los paneles. Sin dockspace, el area de trabajo entera.
    const ImGuiViewport* vp      = ImGui::GetMainViewport();
    const ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(dockId);
    const ImVec2 pos  = central ? central->Pos  : vp->WorkPos;
    const ImVec2 size = central ? central->Size : vp->WorkSize;
    if (size.x <= 0.0f || size.y <= 0.0f) return;

    const math::ScreenRect rect { pos.x, pos.y, size.x, size.y };
    const glm::mat4 viewProj = ctx.camera.ProjectionMatrix() * ctx.camera.ViewMatrix();

    const std::optional<glm::vec2> pCola  = math::ProjectToScreen(viewProj, cola,  rect);
    const std::optional<glm::vec2> pPunta = math::ProjectToScreen(viewProj, ancla, rect);
    // Un extremo detras de la camara: no se dibuja. Recortar a mano mentiria
    // sobre la direccion, y los angulos del overlay siguen ahi.
    if (!pCola || !pPunta) return;

    const glm::vec2 delta = *pPunta - *pCola;
    const f32       largo = glm::length(delta);
    if (largo < 12.0f) return;   // mas corta que esto en pantalla no se lee

    const glm::vec2 u { delta / largo };
    const glm::vec2 n { -u.y, u.x };

    const f32 puntaLargo = glm::min(20.0f, largo * 0.35f);
    const f32 puntaAncho = puntaLargo * 0.45f;
    const glm::vec2 base = *pPunta - u * puntaLargo;

    const ImU32 borde = IM_COL32(0, 0, 0, 210);
    const ImU32 sol   = IM_COL32(255, 214, 92, 255);

    // Dos pasadas, negro grueso y amarillo fino, para que se lea sobre cualquier
    // fondo: el gizmo va sobre la imagen final y no controla que hay detras.
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    dl->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);

    const ImVec2 a { pCola->x, pCola->y };
    const ImVec2 c { base.x,   base.y   };
    dl->AddLine(a, c, borde, 5.0f);
    dl->AddLine(a, c, sol,   2.5f);

    const ImVec2 t0 { pPunta->x,                 pPunta->y                };
    const ImVec2 t1 { base.x + n.x * puntaAncho, base.y + n.y * puntaAncho };
    const ImVec2 t2 { base.x - n.x * puntaAncho, base.y - n.y * puntaAncho };
    dl->AddTriangleFilled(t0, t1, t2, sol);
    dl->AddTriangle(t0, t1, t2, borde, 2.0f);

    // Un disco en la cola marca de que lado esta el sol.
    dl->AddCircleFilled(a, 5.0f, borde);
    dl->AddCircleFilled(a, 3.0f, sol);

    dl->PopClipRect();
}

} // namespace sandbox
