#include "EditorUI.h"

#include "AuthoringUI.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/BloomPass.h>
#include <efengine/renderer/FxaaPass.h>
#include <efengine/renderer/ShadowPass.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/renderer/Model.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/CameraController.h>
#include <efengine/scene/Node.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/serialization/SceneRegistry.h>
#include <efengine/serialization/SceneSerializer.h>

#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <typeinfo>
#include <vector>

namespace sandbox {

using namespace efengine;

namespace {

    constexpr const char* kScenePath = "assets/scenes/sandbox.efe";

    bool algunBehaviorActivo(const scene::SceneGraph& scene, scene::NodeHandle h) {
        if (!scene.IsValid(h)) return false;
        for (const std::unique_ptr<scene::Behavior>& b : scene.Get(h).behaviors) {
            if (b && b->enabled) return true;
        }
        return false;
    }

    // Prende/apaga todos los behaviors de un nodo.
    void setBehaviorsEnabled(scene::SceneGraph& scene, scene::NodeHandle h, bool on) {
        if (!scene.IsValid(h)) return;
        for (std::unique_ptr<scene::Behavior>& b : scene.Get(h).behaviors) b->enabled = on;
    }

    void drawMainMenuBar(EditorContext& ctx) {
        EditorState& st = ctx.state;

        if (!ImGui::BeginMainMenuBar()) return;

        if (ImGui::BeginMenu("Escena")) {
            if (ImGui::MenuItem("Guardar escena")) {
                serialization::SceneSerializer::Save(kScenePath, ctx.scene, ctx.assets, ctx.rm, ctx.registry);
            }
            if (ImGui::MenuItem("Cargar escena")) {
                // Load ya hace Clear() de grafo y assets: no hay que limpiar a mano.
                if (serialization::SceneSerializer::Load(kScenePath, ctx.scene, ctx.assets, ctx.rm, ctx.registry)) {
                    RefreshHandles(ctx);
                }
            }
            ImGui::Separator();
            // MenuItem con bool* devuelve true en el frame en que cambia, igual que Checkbox:
            // asi el toggle individual de cada behavior en el inspector no se pisa cada frame.
            if (ImGui::MenuItem("Animate", nullptr, &st.animate)) {
                setBehaviorsEnabled(ctx.scene, st.rat,        st.animate);
                setBehaviorsEnabled(ctx.scene, st.orbitLight, st.animate);
            }
            if (ImGui::MenuItem("Animar Sol", nullptr, &st.animateSun)) {
                setBehaviorsEnabled(ctx.scene, st.sun, st.animateSun);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Salir", "Esc")) ctx.app.Close();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Ventanas")) {
            ImGui::MenuItem("Jerarquia",        nullptr, &st.showHierarchy);
            ImGui::MenuItem("Inspector",        nullptr, &st.showInspector);
            ImGui::MenuItem("Materiales",       nullptr, &st.showMaterials);
            ImGui::MenuItem("Render",           nullptr, &st.showRender);
            ImGui::MenuItem("Overlay de debug", nullptr, &st.showStats);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void drawHierarchy(EditorContext& ctx) {
        EditorState& st = ctx.state;

        if (!ImGui::Begin("Jerarquia", &st.showHierarchy)) { ImGui::End(); return; }

        std::function<void(scene::NodeHandle)> drawNode = [&](scene::NodeHandle h) {
            scene::Node& node = ctx.scene.Get(h);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                                     | ImGuiTreeNodeFlags_SpanAvailWidth
                                     | ImGuiTreeNodeFlags_DefaultOpen;
            if (node.children.empty())
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (h == st.selected) flags |= ImGuiTreeNodeFlags_Selected;

            // Marca de que adjunto tiene el nodo: malla, luz, o si es solo transform.
            const char* tag = node.mesh ? "[M]" : (node.light ? "[L]" : "[T]");
            // Los behaviors no son exclusivos con malla/luz, van como sufijo aparte.
            const char* behTag = node.behaviors.empty() ? "" : " [B]";

            ImGui::PushID((int)h.index);
            const bool open = ImGui::TreeNodeEx("nodo", flags, "%s %s%s", tag, node.name.c_str(), behTag);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) st.selected = h;

            if (open && !node.children.empty()) {
                const std::vector<scene::NodeHandle> kids = node.children;
                for (scene::NodeHandle c : kids) {
                    if (ctx.scene.IsValid(c)) drawNode(c);
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        };
        drawNode(ctx.scene.Root());

        ImGui::Separator();

        if (ctx.scene.IsValid(st.selected)) {
            if (ImGui::Button("Crear hijo")) {
                char name[32];
                std::snprintf(name, sizeof(name), "nodo_%u", st.spawnCounter++);
                ctx.scene.CreateChild(st.selected, name);
            }
            if (st.selected != ctx.scene.Root()) {
                ImGui::SameLine();
                if (ImGui::Button("Destruir")) {
                    ctx.scene.Destroy(st.selected);
                    st.selected = scene::NodeHandle{};
                }
            }
        } else {
            ImGui::TextDisabled("Ningun nodo seleccionado");
        }

        ImGui::End();
    }

    void drawInspector(EditorContext& ctx) {
        EditorState& st = ctx.state;

        if (!ImGui::Begin("Inspector", &st.showInspector)) { ImGui::End(); return; }

        if (!ctx.scene.IsValid(st.selected)) {
            ImGui::TextDisabled("Ningun nodo seleccionado");
            ImGui::End();
            return;
        }

        scene::Node& node = ctx.scene.Get(st.selected);
        ImGui::Text("Nodo: %s", node.name.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Focus")) FocusSelection(ctx);
        ImGui::TextDisabled("handle { index=%u, gen=%u }", node.self.index, node.self.generation);

        // El world es derivado: se muestra como lectura, no se edita.
        const glm::vec3 worldPos = glm::vec3(node.worldMatrix[3]);
        ImGui::TextDisabled("mundo: %.2f, %.2f, %.2f", worldPos.x, worldPos.y, worldPos.z);

        ImGui::SeparatorText("Transform");
        math::Transform t = node.local;
        bool changed = false;
        changed |= ImGui::DragFloat3("Posicion local", glm::value_ptr(t.position), 0.1f);
        changed |= ImGui::DragFloat3("Rotacion local", glm::value_ptr(t.rotation), 0.5f);
        changed |= ImGui::DragFloat3("Escala local",   glm::value_ptr(t.scale),    0.05f);
        if (changed) ctx.scene.SetLocalTransform(st.selected, t);

        DrawMeshSection(ctx, st.selected);

        if (!node.behaviors.empty()) {
            ImGui::SeparatorText("Behaviors");
            for (usize i = 0; i < node.behaviors.size(); ++i) {
                const char* raw    = typeid(*node.behaviors[i]).name();
                const char* lastNs = std::strrchr(raw, ':');
                const char* label  = lastNs ? lastNs + 1 : raw;

                ImGui::PushID((int)i);
                ImGui::Checkbox(label, &node.behaviors[i]->enabled);
                ImGui::PopID();
            }
        }

        if (node.light) {
            ImGui::SeparatorText("Luz");
            const char* kindName = node.light->kind == scene::LightKind::Point ? "Point" : "Directional";
            ImGui::TextDisabled("tipo: %s", kindName);
            const f32 maxInt = node.light->kind == scene::LightKind::Point ? 20000.0f : 20.0f;
            const f32 speed  = node.light->kind == scene::LightKind::Point ? 10.0f    : 0.05f;
            ImGui::DragFloat3("Color/Int", glm::value_ptr(node.light->color), speed, 0.0f, maxInt);
        }

        ImGui::End();
    }

    void drawRenderPanel(EditorContext& ctx) {
        EditorState& st = ctx.state;

        if (!ImGui::Begin("Render", &st.showRender)) { ImGui::End(); return; }

        if (ImGui::CollapsingHeader("Iluminacion", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Ambient", &ctx.scene.ambientFactor, 0.0f, 1.0f);
            f32 exposure = ctx.camera.Exposure();
            if (ImGui::SliderFloat("Exposure", &exposure, 0.0f, 5.0f)) ctx.camera.SetExposure(exposure);
        }

        if (ImGui::CollapsingHeader("Camara")) {
            scene::CameraSettings& cs = ctx.controller.settings();
            ImGui::SliderFloat("Velocidad",     &cs.moveSpeed,       1.0f,    200.0f);
            ImGui::SliderFloat("Boost (Shift)", &cs.boostMultiplier, 1.0f,    20.0f);
            ImGui::SliderFloat("Sensibilidad",  &cs.lookSensitivity, 0.0005f, 0.02f,  "%.4f");
            ImGui::SliderFloat("Paneo",         &cs.panSpeed,        0.0001f, 0.01f,  "%.4f");
            ImGui::SliderFloat("Dolly",         &cs.dollySpeed,      0.01f,   0.5f);
            ImGui::Checkbox("Invertir X", &cs.invertX);
            ImGui::SameLine();
            ImGui::Checkbox("Invertir Y", &cs.invertY);

            ImGui::TextDisabled("mouselook: %s", ctx.controller.LookToggled() ? "ON (Tab/Esc para salir)"
                                                                             : "off (Tab para entrar)");
            ImGui::TextDisabled("pivote a %.2f | yaw %.1f | pitch %.1f",
                                ctx.controller.PivotDistance(),
                                glm::degrees(ctx.controller.Yaw()),
                                glm::degrees(ctx.controller.Pitch()));

            // Los bindings son invisibles: nadie los adivina mirando la ventana.
            ImGui::SeparatorText("Controles");
            ImGui::TextDisabled("Tab mirar libre  -  RMB mirar sostenido");
            ImGui::TextDisabled("Alt+LMB orbitar  -  MMB panear");
            ImGui::TextDisabled("scroll dolly  -  WASD volar  -  Q/E bajar-subir");
            ImGui::TextDisabled("Shift acelerar  -  F encuadrar seleccion");
        }

        if (ImGui::CollapsingHeader("Bloom")) {
            renderer::BloomSettings& s = ctx.app.GetBloomPass().settings();
            ImGui::SliderFloat("Threshold",  &s.threshold,  0.0f, 5.0f);
            ImGui::SliderFloat("Knee",       &s.knee,       0.0f, 1.0f);
            ImGui::SliderFloat("Intensity",  &s.intensity,  0.0f, 0.5f);
            ImGui::SliderInt  ("Iterations", &s.iterations, 1,    10);
        }

        if (ImGui::CollapsingHeader("FXAA")) {
            renderer::FxaaSettings& fx = ctx.app.GetFxaaPass().settings();
            ImGui::Checkbox("FXAA habilitado", &fx.enabled);
        }

        if (ImGui::CollapsingHeader("Sombras", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderer::ShadowSettings& sh = ctx.app.GetShadowPass().settings();
            ImGui::Checkbox   ("Habilitadas",    &sh.enabled);
            ImGui::SliderFloat("Ortho HalfSize", &sh.orthoHalfSize, 5.0f,  200.0f);
            ImGui::SliderFloat("Distancia Luz",  &sh.distance,      10.0f, 400.0f);
            ImGui::SliderFloat("Near",           &sh.nearPlane,     0.1f,  50.0f);
            ImGui::SliderFloat("Far",            &sh.farPlane,      50.0f, 600.0f);
            ImGui::SliderFloat("Bias Min",       &sh.biasMin,       0.0f,  0.01f, "%.4f");
            ImGui::SliderFloat("Bias Max",       &sh.biasMax,       0.0f,  0.02f, "%.4f");
        }

        ImGui::End();
    }

    void drawStatsOverlay(EditorContext& ctx) {
        EditorState& st = ctx.state;

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        // WorkPos ya descuenta la barra de menu, asi que el overlay no queda debajo.
        const ImVec2 pos { vp->WorkPos.x + 10.0f, vp->WorkPos.y + 10.0f };
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f);

        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                                     | ImGuiWindowFlags_NoMove
                                     | ImGuiWindowFlags_NoSavedSettings
                                     | ImGuiWindowFlags_NoFocusOnAppearing
                                     | ImGuiWindowFlags_NoNav
                                     | ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("##stats", &st.showStats, flags)) {
            const FrameStats& fs = st.stats;
            ImGui::Text("%6.1f FPS   (%.2f ms)", fs.AvgFps(), fs.AvgMs());

            // Escala fija 0..33.3 ms (o sea, hasta 30 FPS). Autoescalada, el ruido
            // de un frame quieto se veria como una montana rusa y no informaria nada.
            ImGui::PlotLines("##ms", fs.History(), fs.HistoryCount(), fs.HistoryOffset(),
                             nullptr, 0.0f, 33.3f, ImVec2(220.0f, 40.0f));

            ImGui::Separator();
            const glm::vec3& p = ctx.camera.Position();
            ImGui::Text("cam  %.2f, %.2f, %.2f", p.x, p.y, p.z);
            ImGui::Text("exp  %.2f   asp %.2f", ctx.camera.Exposure(), ctx.app.GetWindow().GetAspectRatio());
        }
        ImGui::End();
    }

} // namespace

void RefreshHandles(EditorContext& ctx) {
    EditorState& st = ctx.state;

    st.selected   = scene::NodeHandle{};  // los handles viejos quedaron invalidos
    st.meshError.clear();   // el error viejo hablaba de una escena que ya no existe

    // Los indices de material de la escena vieja no significan nada en la nueva.
    st.activeMaterial = resources::SceneAssets::kInvalidIndex;
    st.draftFor       = resources::SceneAssets::kInvalidIndex;
    st.materialError.clear();
    st.rat        = ctx.scene.FindByName("model_rata");
    st.orbitLight = ctx.scene.FindByName("luz_animada");
    st.sun        = ctx.scene.FindByName("directional_light");
    st.animate    = algunBehaviorActivo(ctx.scene, st.rat);
    st.animateSun = algunBehaviorActivo(ctx.scene, st.sun);
}

void FocusSelection(EditorContext& ctx) {
    EditorState& st = ctx.state;
    if (!ctx.scene.IsValid(st.selected)) return;

    const scene::Node& node = ctx.scene.Get(st.selected);

    // node.worldMatrix es derivado y se recalcula en scene.Update(), que en el
    // loop corre DESPUES de esto: se usa la matriz del frame anterior. Un frame
    // de retraso en un encuadre no se percibe.
    if (node.mesh && node.mesh->model && node.mesh->model->bounds().Valid()) {
        const renderer::AABB mundo = node.mesh->model->bounds().Transformed(node.worldMatrix);
        ctx.controller.Focus(mundo.Center(), mundo.Radius());
    } else {
        // Nodo pelado, luz, o modelo sin submeshes: no hay AABB del que sacar
        // un radio, asi que centrar es lo unico que se puede hacer.
        ctx.controller.Focus(glm::vec3(node.worldMatrix[3]));
    }
}

void DrawEditor(EditorContext& ctx) {
    ctx.state.stats.Push(ctx.app.DeltaTime());

    drawMainMenuBar(ctx);
    if (ctx.state.showHierarchy) drawHierarchy(ctx);
    if (ctx.state.showInspector) drawInspector(ctx);
    if (ctx.state.showMaterials) DrawMaterialsPanel(ctx);
    if (ctx.state.showRender)    drawRenderPanel(ctx);
    if (ctx.state.showStats)     drawStatsOverlay(ctx);
}

} // namespace sandbox
