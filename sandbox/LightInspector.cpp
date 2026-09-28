#include "LightInspector.h"

#include "EditorUI.h"
#include "panels/PanelUI.h"

#include <efengine/math/ColorTemperature.h>
#include <efengine/scene/Node.h>
#include <efengine/scene/SceneGraph.h>

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include <algorithm>

namespace sandbox {

using namespace efengine;

namespace {

    scene::LightAttachment luzPorDefecto(scene::LightKind kind) {
        scene::LightAttachment a;
        a.kind = kind;
        a.intensity = (kind == scene::LightKind::Directional) ? 3.0f : 100.0f;
        return a;
    }

    // Mismo orden que scene::LightKind.
    const char* kTipos[] = { "Point", "Directional", "Spot" };

} // namespace

void DrawLightSection(EditorContext& ctx, scene::NodeHandle handle) {
    ImGui::SeparatorText("Luz");
    scene::Node& node = ctx.scene.Get(handle);

    if (!node.light) {
        // Tres botones y no combo + boton: no hay estado que recordar entre frames.
        const f32 ancho = (ImGui::GetContentRegionAvail().x - 2.0f * ImGui::GetStyle().ItemSpacing.x) / 3.0f;
        if (ImGui::Button("+ Point", ImVec2(ancho, 0.0f))) {
            ctx.scene.AttachLight(handle, luzPorDefecto(scene::LightKind::Point));
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Spot", ImVec2(ancho, 0.0f))) {
            ctx.scene.AttachLight(handle, luzPorDefecto(scene::LightKind::Spot));
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Directional", ImVec2(ancho, 0.0f))) {
            ctx.scene.AttachLight(handle, luzPorDefecto(scene::LightKind::Directional));
        }
        return;
    }

    scene::LightAttachment& l = *node.light;

    int tipo = static_cast<int>(l.kind);
    if (ImGui::Combo("Tipo", &tipo, kTipos, IM_ARRAYSIZE(kTipos))) {
        l.kind = static_cast<scene::LightKind>(tipo);
    }

    ImGui::ColorEdit3("Color", glm::value_ptr(l.color));
    // Velocidad proporcional: arrastrar de 5 a 6 y de 5000 a 6000 cuesta lo mismo.
    ImGui::DragFloat("Intensidad", &l.intensity, std::max(l.intensity * 0.01f, 0.01f),
                     0.0f, 100000.0f, "%.2f");

    ImGui::Checkbox("Temperatura", &l.useTemperature);
    if (l.useTemperature) {
        ImGui::SliderFloat("Kelvin", &l.temperatureK, math::kMinKelvin, 12000.0f, "%.0f K");
        const glm::vec3 k = math::KelvinToLinearRgb(l.temperatureK);
        const f32 m = std::max(k.r, std::max(k.g, k.b));
        ImGui::SameLine();
        ImGui::ColorButton("##tono", ImVec4(k.r / m, k.g / m, k.b / m, 1.0f),
                           ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker);
    }

    if (l.kind != scene::LightKind::Directional) {
        ImGui::DragFloat("Rango",        &l.range,        0.1f,   0.01f, 1000.0f, "%.2f m");
        ImGui::DragFloat("Radio fuente", &l.sourceRadius, 0.005f, 0.0f,  5.0f,    "%.3f m");
    }
    if (l.kind == scene::LightKind::Spot) {
        ImGui::SliderFloat("Cono exterior", &l.outerConeDeg, 1.0f, 89.0f,          "%.1f deg");
        ImGui::SliderFloat("Cono interior", &l.innerConeDeg, 0.0f, l.outerConeDeg, "%.1f deg");
        l.innerConeDeg = std::min(l.innerConeDeg, l.outerConeDeg);
    }

    ImGui::BeginDisabled(true);
    ImGui::Checkbox("Proyecta sombra", &l.castShadows);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Llega en el ciclo 2: sombras de luces locales");
    }

    if (l.kind == scene::LightKind::Directional) {
        if (ctx.scene.PrimarySun() == handle) {
            ImGui::TextDisabled("es el sol primario (sombras por cascadas)");
        } else if (ImGui::Button("Hacer sol primario", ImVec2(-kAnchoEtiqueta, 0.0f))) {
            ctx.scene.SetPrimarySun(handle);
        }
    }

    if (ImGui::Button("Quitar luz", ImVec2(-kAnchoEtiqueta, 0.0f))) ctx.scene.DetachLight(handle);
}

} // namespace sandbox
