#include "panels/PassPanel.h"
#include "panels/PanelUI.h"

#include "EditorUI.h"
#include "OscillatorBehavior.h"
#include "SceneHelpers.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/ClusterLightPass.h>
#include <efengine/renderer/FrameView.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/renderer/TaaPass.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/Node.h>
#include <efengine/scene/SceneGraph.h>

#include <imgui.h>

#include <memory>
#include <string>

namespace sandbox {

using namespace efengine;

namespace {

    f32 g_amplitud   = 2.0f;
    f32 g_frecuencia = 0.5f;

    glm::vec3 derechaDeCamara(const scene::Camera& c) {
        const glm::vec3 adelante = glm::normalize(c.Target() - c.Position());
        return glm::cross(adelante, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    OscillatorBehavior* oscilador(scene::Node& n) {
        for (std::unique_ptr<scene::Behavior>& b : n.behaviors)
            if (OscillatorBehavior* o = dynamic_cast<OscillatorBehavior*>(b.get())) return o;
        return null;
    }

    // Vuelve a dejar el nodo donde estaba antes de oscilar.
    void quitarOscilador(EditorContext& ctx, scene::NodeHandle h) {
        scene::Node& n = ctx.scene.Get(h);
        for (auto it = n.behaviors.begin(); it != n.behaviors.end(); ++it) {
            OscillatorBehavior* o = dynamic_cast<OscillatorBehavior*>(it->get());
            if (o == null) continue;
            if (o->HasBase()) {
                math::Transform t = n.local;
                t.position = o->Base();
                ctx.scene.SetLocalTransform(h, t);
            }
            n.behaviors.erase(it);
            return;
        }
    }

    void cuboDePrueba(EditorContext& ctx) {
        static u32 contador = 0u;
        const std::string nombre = "taa_cubo_" + std::to_string(contador++);

        const u32 mat = MaterialPlano(ctx, nombre.c_str(), glm::vec3(0.8f, 0.3f, 0.1f), 0.4f);
        if (mat == resources::SceneAssets::kInvalidIndex) return;

        renderer::BoxParams p;
        p.half   = glm::vec3(0.5f);
        p.inward = 0u;
        const renderer::Model* modelo = AgregarCaja(ctx, p);
        if (modelo == null) return;

        const glm::vec3 adelante = glm::normalize(ctx.camera.Target() - ctx.camera.Position());
        math::Transform t;
        t.position = ctx.camera.Position() + adelante * 5.0f;
        const scene::NodeHandle h = NodoConMalla(ctx, nombre.c_str(), modelo, t,
                                                 MaterialEnTodasLasCaras(ctx.assets.MaterialAt(mat)));
        ctx.scene.Get(h).behaviors.push_back(
            std::make_unique<OscillatorBehavior>(derechaDeCamara(ctx.camera), g_amplitud, g_frecuencia));
        ctx.state.selected = h;
    }

}

    void dibujarPanelTemporal(EditorContext& ctx) {
        if (!ImGui::CollapsingHeader("Temporal (TAA)")) return;

        if (renderer::TaaPass* taa = ctx.app.GetPipeline().Find<renderer::TaaPass>()) {
            ImGui::Checkbox("TAA", &taa->enabled);
            ImGui::SameLine();
            ImGui::TextDisabled("(arrastra el jitter)");
            renderer::TaaSettings& s = taa->settings();
            ImGui::SliderFloat("Alfa (peso del frame actual)", &s.alpha, renderer::kTaaMinAlpha, 0.5f);
            ImGui::Checkbox("Ver velocidades", &s.debugVelocity);
            if (ImGui::Button("Reset historia")) taa->ResetHistory();
        } else {
            ImGui::TextDisabled("TaaPass no disponible");
        }

        const renderer::FrameView& v = ctx.app.LastFrameView();
        const f32 pxX = v.jitterNdc.x * 0.5f * static_cast<f32>(v.width);
        const f32 pxY = v.jitterNdc.y * 0.5f * static_cast<f32>(v.height);
        ImGui::Text("frame %u   jitter (%.3f, %.3f) px", v.frameIndex, pxX, pxY);

        if (renderer::ClusterLightPass* c = ctx.app.GetPipeline().Find<renderer::ClusterLightPass>()) {
            ImGui::Text("reconstrucciones de AABBs de clusters: %u", c->aabbRebuilds());
            ImGui::TextDisabled("tiene que quedar quieto con el jitter prendido");
        }

        ImGui::SeparatorText("Objeto movil");
        ImGui::SliderFloat("Amplitud (m)",    &g_amplitud,   0.1f,  10.0f);
        ImGui::SliderFloat("Frecuencia (Hz)", &g_frecuencia, 0.05f, 3.0f);
        if (ImGui::Button("Cubo de prueba")) cuboDePrueba(ctx);

        const scene::NodeHandle sel = ctx.state.selected;
        if (!ctx.scene.IsValid(sel)) {
            ImGui::TextDisabled("sin nodo seleccionado");
            return;
        }
        scene::Node& n = ctx.scene.Get(sel);
        OscillatorBehavior* o = oscilador(n);
        bool oscila = (o != null);
        if (ImGui::Checkbox("Oscilar seleccionado", &oscila)) {
            if (oscila) {
                n.behaviors.push_back(
                    std::make_unique<OscillatorBehavior>(derechaDeCamara(ctx.camera), g_amplitud, g_frecuencia));
            } else {
                quitarOscilador(ctx, sel);
            }
        } else if (o != null) {
            o->amplitude = g_amplitud;
            o->frequency = g_frecuencia;
        }
    }

}
