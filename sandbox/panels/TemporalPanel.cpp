#include "panels/PassPanel.h"
#include "panels/PanelUI.h"

#include "EditorUI.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/ClusterLightPass.h>
#include <efengine/renderer/FrameView.h>
#include <efengine/renderer/ScenePipeline.h>

#include <imgui.h>

namespace sandbox {

using namespace efengine;

    void dibujarPanelTemporal(EditorContext& ctx) {
        if (!ImGui::CollapsingHeader("Temporal (debug)")) return;

        renderer::TemporalSettings& s = ctx.app.GetTemporalSettings();
        ImGui::Checkbox("Jitter (debug)", &s.jitter);
        ImGui::TextDisabled("sin resolve la imagen tiembla un subpixel: es lo esperado");

        const renderer::FrameView& v = ctx.app.LastFrameView();
        const f32 pxX = v.jitterNdc.x * 0.5f * static_cast<f32>(v.width);
        const f32 pxY = v.jitterNdc.y * 0.5f * static_cast<f32>(v.height);
        ImGui::Text("frame %u   jitter (%.3f, %.3f) px", v.frameIndex, pxX, pxY);

        if (renderer::ClusterLightPass* c = ctx.app.GetPipeline().Find<renderer::ClusterLightPass>()) {
            ImGui::Text("reconstrucciones de AABBs de clusters: %u", c->aabbRebuilds());
            ImGui::TextDisabled("tiene que quedar quieto con el jitter prendido");
        }
    }

}
