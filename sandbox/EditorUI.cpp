#include "EditorUI.h"

#include "AuthoringUI.h"
#include "TestScene.h"
#include "SunGizmo.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/BloomPass.h>
#include <efengine/renderer/FxaaPass.h>
#include <efengine/renderer/ShadowPass.h>
#include <efengine/renderer/DdgiPass.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/DdgiVolume.h>
#include <efengine/renderer/Bounds.h>
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
#include <imgui_internal.h>   // DockBuilder*: API de layout, no esta en imgui.h

#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
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

    // Arma el layout por defecto: Jerarquia a la izquierda, Inspector y Render
    // como pestanas a la derecha, Materiales abajo, y el centro libre para la
    // escena 3D.
    //
    // Solo corre cuando no hay layout que respetar, o cuando lo piden explicitamente.
    // Si corriera siempre pisaria cada frame el acomodo que el usuario dejo guardado.
    //
    // "No hay layout" es nodo inexistente (primer arranque, imgui.ini borrado) O nodo
    // vacio: el ini tambien persiste el dockspace pelado, sin nada adentro, y en ese
    // caso el nodo existe pero no dice nada. Con los paneles cerrados a mano queda
    // vacio y esto reconstruye cada frame; es un estado degenerado y sale barato.
    void ensureDefaultLayout(ImGuiID dockId, bool force) {
        const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockId);
        if (!force && node != nullptr && !node->IsEmpty()) return;

        ImGui::DockBuilderRemoveNode(dockId);   // limpia lo que hubiera
        ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_DockSpace
                                        | ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::DockBuilderSetNodeSize(dockId, ImGui::GetMainViewport()->WorkSize);

        // Cada split devuelve el nodo nuevo y deja en 'center' lo que sobra, asi
        // que las fracciones son sobre el area que queda, no sobre el total.
        ImGuiID center = dockId;
        const ImGuiID left   = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.20f, nullptr, &center);
        const ImGuiID right  = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.28f, nullptr, &center);
        const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down,  0.28f, nullptr, &center);

        ImGui::DockBuilderDockWindow("Jerarquia",  left);
        // Inspector primero: el primero que se dockea en un nodo queda como pestana
        // activa, y al abrir es lo que se quiere ver.
        ImGui::DockBuilderDockWindow("Inspector",  right);
        ImGui::DockBuilderDockWindow("Render",     right);
        ImGui::DockBuilderDockWindow("Materiales", bottom);

        ImGui::DockBuilderFinish(dockId);
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
            if (ImGui::MenuItem("Sala de Cornell")) {
                BuildCornellScene(ctx);   // hace Clear de escena y assets, y RefreshHandles
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
            ImGui::MenuItem("UI",               nullptr, &st.showUI);
            ImGui::Separator();
            ImGui::MenuItem("Jerarquia",        nullptr, &st.showHierarchy);
            ImGui::MenuItem("Inspector",        nullptr, &st.showInspector);
            ImGui::MenuItem("Materiales",       nullptr, &st.showMaterials);
            ImGui::MenuItem("Render",           nullptr, &st.showRender);
            ImGui::MenuItem("Overlay de debug", nullptr, &st.showStats);
            ImGui::Separator();
            if (ImGui::MenuItem("Restablecer layout")) {
                st.resetLayout = true;
                // Un panel cerrado no se puede redockear: hay que reabrirlos todos
                // o el layout nuevo tendria huecos.
                st.showHierarchy = st.showInspector = st.showMaterials = st.showRender = true;
            }
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

    void drawDdgiSection(EditorContext& ctx) {
        if (!ImGui::CollapsingHeader("DDGI (iluminacion indirecta)")) return;

        std::optional<renderer::DdgiPass>& opt = ctx.app.GetDdgiPass();
        if (!opt.has_value()) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                               "DdgiPass no disponible: fallo la carga de shaders.");
            ImGui::TextWrapped("La escena esta usando IBL puro. Mira la consola.");
            return;
        }
        renderer::DdgiPass&     pass = *opt;
        renderer::DdgiSettings& s    = pass.settings();

        // -- Lo primero que hay que mirar cuando "no se ve la GI" --------------
        if (pass.atlasValid()) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "pbr.frag recibe los atlas: SI");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "pbr.frag recibe los atlas: NO");
            ImGui::TextWrapped("Hasta que corra un blend, DDGI aporta cero y la imagen es IBL puro.");
        }

        ImGui::Checkbox("Habilitado", &s.enabled);

        // -- Grilla ------------------------------------------------------------
        ImGui::SeparatorText("Grilla");
        bool gridChanged = false;
        gridChanged |= ImGui::DragFloat3("Origen",        &s.grid.origin.x,  0.1f);
        gridChanged |= ImGui::DragFloat3("Espaciado",     &s.grid.spacing.x, 0.05f, 0.05f, 10.0f);
        gridChanged |= ImGui::DragInt3  ("Probes por eje", &s.grid.counts.x, 1.0f,
                                         1, renderer::kMaxProbesPerAxis);
        ImGui::Text("Total: %u probes", renderer::ProbeCount(s.grid));

        // Encajar la grilla a la escena resuelve de un click la clase entera de
        // bug "la grilla no cubre la sala", que es con la que arranco este ciclo.
        if (ImGui::Button("Encajar grilla a la escena")) {
            const renderer::AABB& b = ctx.scene.WorldBounds();
            if (b.Valid()) {
                // Un 10% de margen hacia adentro: un probe DENTRO de una pared
                // captura su interior y contamina a sus vecinos por el peso
                // trilineal.
                const glm::vec3 ext    = b.Extents() * 0.9f;
                const glm::vec3 minPos = b.Center() - ext;
                const glm::ivec3 n     = s.grid.counts;
                s.grid.origin  = minPos;
                s.grid.spacing = glm::vec3(
                    n.x > 1 ? (2.0f * ext.x) / f32(n.x - 1) : 1.0f,
                    n.y > 1 ? (2.0f * ext.y) / f32(n.y - 1) : 1.0f,
                    n.z > 1 ? (2.0f * ext.z) / f32(n.z - 1) : 1.0f);
                gridChanged = true;
            }
        }
        if (gridChanged) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f),
                               "Cambiar la grilla realoca los atlas y reinicia el barrido.");
        }

        // -- Update ------------------------------------------------------------
        ImGui::SeparatorText("Update");
        int perFrame = static_cast<int>(s.probesPerFrame);
        if (ImGui::SliderInt("Probes por frame", &perFrame, 0,
                             static_cast<int>(renderer::kMaxProbesPerFrame))) {
            s.probesPerFrame = static_cast<u32>(perFrame);
        }
        ImGui::SliderFloat("Histeresis", &s.hysteresis, 0.0f, 0.995f, "%.3f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Cuanto del valor viejo se conserva. ESTO es el denoise\n"
                              "temporal de DDGI: no hace falta un denoiser aparte.\n"
                              "Mas alto = mas estable y mas lento en reaccionar.");
        }
        ImGui::Checkbox("Congelar (freeze)", &s.freeze);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) pass.Reset();

        const u32 total = renderer::ProbeCount(s.grid);
        const u32 framesPorBarrido = (s.probesPerFrame > 0u)
                                   ? (total + s.probesPerFrame - 1u) / s.probesPerFrame
                                   : 0u;
        ImGui::Text("Cursor: %u / %u   Barridos: %u", pass.cursor(), total, pass.sweepsDone());
        ImGui::Text("Frames por barrido: %u", framesPorBarrido);
        ImGui::Text("Pase (CPU): %.3f ms", pass.lastMs());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Tiempo de CPU emitiendo las llamadas, no de GPU\n"
                              "ejecutandolas. Sirve para detectar que el round-robin\n"
                              "se fue de escala, no como profiler.");
        }

        // -- Sampleo -----------------------------------------------------------
        ImGui::SeparatorText("Sampleo");
        ImGui::SliderFloat("Intensidad", &s.intensity, 0.0f, 4.0f);
        ImGui::SliderFloat("Normal bias", &s.normalBias, 0.0f, 1.0f, "%.3f m");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Sube si la luz atraviesa las paredes.\n"
                              "Baja si los rincones tienen una banda oscura.");
        }
        ImGui::SliderFloat("View bias", &s.viewBias, 0.0f, 1.0f, "%.3f m");
        ImGui::SliderFloat("Chebyshev sharpness", &s.chebyshevSharpness, 1.0f, 16.0f);

        // El rango sale de la escena, no de un numero fijo: con un tope de 100 m
        // fijo, abrir el panel con maxDistance en 200 lo clamparia en silencio y
        // cambiaria el far plane de la captura sin que nadie toque nada.
        const renderer::AABB& bounds = ctx.scene.WorldBounds();
        const f32 topeDist = bounds.Valid() ? glm::max(4.0f * bounds.Radius(), 10.0f) : 200.0f;
        ImGui::SliderFloat("Distancia max (captura)", &s.maxDistance, 1.0f, topeDist, "%.1f m");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Far plane de la captura de probes. Muy alto tira la\n"
                              "precision del depth; muy bajo deja la captura vacia.");
        }

        // -- Debug -------------------------------------------------------------
        ImGui::SeparatorText("Debug");
        ImGui::Checkbox("Mostrar probes", &s.debugProbes);
        const char* modos[] = { "Irradiancia", "Media de distancia", "Target de captura",
                                "Target de captura (distancia)" };
        int modo = static_cast<int>(s.debugMode);
        if (ImGui::Combo("Modo", &modo, modos, 4)) s.debugMode = static_cast<u32>(modo);
        ImGui::SliderFloat("Radio de esfera", &s.debugRadius, 0.02f, 0.5f, "%.3f m");
    }

    void drawRenderPanel(EditorContext& ctx) {
        EditorState& st = ctx.state;

        if (!ImGui::Begin("Render", &st.showRender)) { ImGui::End(); return; }

        if (ImGui::CollapsingHeader("Iluminacion", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Intensidad IBL", &ctx.scene.iblIntensity, 0.0f, 2.0f);
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

        drawDdgiSection(ctx);

        ImGui::End();
    }

    void drawStatsOverlay(EditorContext& ctx, ImGuiID dockId) {
        EditorState& st = ctx.state;

        // El overlay va sobre la escena, no sobre los paneles: se ancla al nodo
        // central del dockspace (el agujero passthrough). Si todavia no existe
        // (primer frame), cae al area de trabajo del viewport.
        const ImGuiViewport*  vp      = ImGui::GetMainViewport();
        const ImGuiDockNode*  central = ImGui::DockBuilderGetCentralNode(dockId);
        const ImVec2 origin = central ? central->Pos : vp->WorkPos;

        ImGui::SetNextWindowPos({ origin.x + 10.0f, origin.y + 10.0f }, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f);

        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                                     | ImGuiWindowFlags_NoDocking
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

    if (!ctx.state.showUI) return;

    // Y el dockspace antes que los paneles: cada Begin() de abajo consulta en que
    // nodo esta dockeado, y ese nodo tiene que existir ya.
    application::DebugUI& ui = ctx.app.GetDebugUI();
    const ImGuiID dockId = ui.BeginDockspace();
    ensureDefaultLayout(dockId, ctx.state.resetLayout);
    ctx.state.resetLayout = false;
    ui.EndDockspace(dockId);

    if (ctx.state.showHierarchy) drawHierarchy(ctx);
    if (ctx.state.showInspector) drawInspector(ctx);
    if (ctx.state.showMaterials) DrawMaterialsPanel(ctx);
    if (ctx.state.showRender)    drawRenderPanel(ctx);
    if (ctx.state.showStats)     drawStatsOverlay(ctx, dockId);

    DrawSunGizmo(ctx, dockId);
}

} // namespace sandbox
