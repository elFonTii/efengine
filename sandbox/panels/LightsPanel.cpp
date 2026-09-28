#include "panels/PassPanel.h"
#include "panels/PanelUI.h"

#include "EditorUI.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/ClusterLightPass.h>
#include <efengine/renderer/LightUploadPass.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/renderer/ShaderBlocks.h>

#include <imgui.h>

namespace sandbox {

using namespace efengine;

    void dibujarPanelLuces(EditorContext& ctx) {
        if (!ImGui::CollapsingHeader("Luces (clustered)")) return;

        CamposAlineados alineados;

        if (renderer::LightUploadPass* subida = ctx.app.GetPipeline().Find<renderer::LightUploadPass>()) {
            const renderer::LightStats& s = subida->stats();
            ImGui::SeparatorText("Escena");
            ImGui::Text("locales        %u (tope %u)", s.locals, renderer::kMaxLocalLights);
            ImGui::Text("visibles       %u", s.visible);
            ImGui::Text("direccionales  %u (tope %u)", s.directional, renderer::kMaxDirectionalLights);
            if (s.droppedLocals > 0u || s.droppedDirectional > 0u) {
                ImGui::TextColored(kColorAviso, "descartadas: %u locales, %u direccionales",
                                   s.droppedLocals, s.droppedDirectional);
            }
        }

        renderer::ClusterLightPass* pase = ctx.app.GetPipeline().Find<renderer::ClusterLightPass>();
        if (pase == null) {
            ImGui::TextColored(kColorError, "ClusterLightPass no disponible: fallo el shader de culling.");
            ImGui::TextWrapped("pbr.frag recorre todas las luces visibles por pixel. Mira la consola.");
            return;
        }

        ImGui::SeparatorText("Grilla");
        // Apagarlo no apaga las luces: pbr.frag recorre todas las visibles. Sirve
        // para medir cuanto ahorra la grilla.
        ImGui::Checkbox("Habilitado", &pase->enabled);

        const renderer::ClusterGrid& g = pase->grid();
        const f64 mb = static_cast<f64>(g.Count()) * (g.maxLightsPerCluster + 1u) * 4.0 / (1024.0 * 1024.0);
        ImGui::TextDisabled("%u x %u x %u = %u clusters, listas %.1f MB",
                            g.tilesX, g.tilesY, g.slices, g.Count(), mb);

        renderer::ClusterSettings& cs = pase->settings();
        int tile   = static_cast<int>(cs.tileSizePx);
        int cortes = static_cast<int>(cs.slices);
        int maximo = static_cast<int>(cs.maxLightsPerCluster);
        if (ImGui::SliderInt("Tile",            &tile,   16, 256, "%d px")) cs.tileSizePx          = static_cast<u32>(tile);
        if (ImGui::SliderInt("Cortes Z",        &cortes,  2,  64))          cs.slices              = static_cast<u32>(cortes);
        if (ImGui::SliderInt("Max por cluster", &maximo,  8, 512))          cs.maxLightsPerCluster = static_cast<u32>(maximo);
        ImGui::DragFloat("Near split", &cs.nearSplit, 0.05f, 0.05f, 50.0f,    "%.2f m");
        ImGui::DragFloat("Far limit",  &cs.farLimit,  5.0f,  10.0f, 20000.0f, "%.0f m");

        const char* kVistas[] = { "Imagen final", "Heatmap de luces", "Cortes Z" };
        int vista = static_cast<int>(cs.debugView);
        if (ImGui::Combo("Vista de debug", &vista, kVistas, IM_ARRAYSIZE(kVistas))) {
            cs.debugView = static_cast<u32>(vista);
        }
        if (cs.debugView == 1u) ImGui::TextDisabled("magenta = cluster que llego al maximo");
    }

}
