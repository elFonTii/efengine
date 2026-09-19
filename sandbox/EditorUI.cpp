#include "EditorUI.h"

#include "AuthoringUI.h"
#include "TestScene.h"
#include "SunGizmo.h"
#include "ColliderGizmo.h"

#include <efengine/application/Application.h>
#include <efengine/core/Log.h>
#include <efengine/core/Time.h>
#include <efengine/gameplay/GameWorld.h>
#include <efengine/renderer/BloomPass.h>
#include <efengine/renderer/FxaaPass.h>
#include <efengine/renderer/GpuProfiler.h>
#include <efengine/renderer/ScenePipeline.h>

#include "panels/PassPanel.h"
#include "panels/PanelUI.h"
#include <efecom/RHI.h>
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
#include <imgui_stdlib.h>     // InputText sobre std::string

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <typeinfo>
#include <vector>

namespace sandbox {

using namespace efengine;

namespace {

    // Donde viven las escenas. El menu lista lo que hay aca y "Guardar como..."
    // escribe aca; el .efe de arranque lo elige main.cpp.
    constexpr const char* kScenesDir = "assets/scenes/";

    // "assets/scenes/interior.efe" -> "interior.efe". Los items del menu muestran
    // el nombre solo: la ruta entera no aporta y hace el menu tres veces mas ancho.
    // FileIO::ListFiles devuelve siempre '/' (generic_string), tambien en Windows,
    // asi que alcanza con buscar la ultima barra.
    std::string nombreDeArchivo(const std::string& ruta) {
        const usize barra = ruta.find_last_of('/');
        return barra == std::string::npos ? ruta : ruta.substr(barra + 1);
    }

    // Carga un .efe y deja el editor consistente.
    //
    // RefreshHandles corre tambien cuando el Load falla, y eso es a proposito: si
    // revienta el parse el grafo queda intacto, pero si revienta el Resolve la
    // escena ya quedo a medio reemplazar y los handles cacheados (rat, sun,
    // orbitLight, selected) apuntan a nodos que ya no existen.
    void cargarEscena(EditorContext& ctx, const std::string& ruta) {
        if (serialization::SceneSerializer::Load(ruta.c_str(), ctx.scene, ctx.assets,
                                                 ctx.rm, ctx.registry)) {
            ctx.state.currentScenePath = ruta;
        } else {
            EF_LOG_ERROR("No se pudo cargar la escena '%s'", ruta.c_str());
            ctx.state.currentScenePath.clear();
        }
        RefreshHandles(ctx);
    }

    // Los colores de mensaje y CamposAlineados viven ahora en
    // panels/PanelUI.h: los comparten los paneles de pases, que estan en
    // archivos propios.

    // Pega el nombre tipeado al directorio de escenas y le pone .efe si falta:
    // guardar un archivo sin extension lo dejaria fuera del listado del menu.
    std::string rutaDeEscena(const std::string& nombre) {
        std::string archivo = nombre;
        const bool tieneExt = archivo.size() >= 4
                           && archivo.compare(archivo.size() - 4, 4, ".efe") == 0;
        if (!tieneExt) archivo += ".efe";
        return std::string(kScenesDir) + archivo;
    }

    // Ya hay un .efe con esa ruta? Se pregunta al catalogo y no al disco porque
    // el catalogo se refresca al abrir el modal, asi que dice lo mismo.
    bool escenaExiste(const EditorState& st, const std::string& ruta) {
        const std::vector<std::string>& escenas = st.catalog.Scenes();
        return std::find(escenas.begin(), escenas.end(), ruta) != escenas.end();
    }

    // Guarda con nombre nuevo. Devuelve false y deja el modal abierto si fallo,
    // asi el nombre tipeado no se pierde.
    bool guardarEscenaComo(EditorContext& ctx, const std::string& ruta) {
        if (!serialization::SceneSerializer::Save(ruta.c_str(), ctx.scene, ctx.assets,
                                                  ctx.rm, ctx.registry)) {
            ctx.state.saveAsError = "No se pudo escribir el archivo. Ver el log.";
            return false;
        }
        ctx.state.currentScenePath = ruta;
        ctx.state.catalog.Rescan();   // que el archivo nuevo aparezca en el submenu "Cargar"
        return true;
    }

    // Dibuja el modal. Va afuera de la barra de menu por el tema del ID stack que
    // explica el comentario de openSaveAs en EditorUI.h.
    void drawSaveAsModal(EditorContext& ctx) {
        EditorState& st = ctx.state;

        if (st.openSaveAs) {
            ImGui::OpenPopup("Guardar escena como");
            st.openSaveAs = false;
        }

        const ImVec2 centro = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(centro, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (!ImGui::BeginPopupModal("Guardar escena como", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize)) return;

        ImGui::Text("Se guarda en %s", kScenesDir);
        ImGui::InputText("Nombre", &st.saveAsName);
        if (!st.saveAsError.empty()) ImGui::TextColored(kColorError, "%s", st.saveAsError.c_str());

        const std::string ruta = rutaDeEscena(st.saveAsName);

        ImGui::BeginDisabled(st.saveAsName.empty());
        if (ImGui::Button("Guardar", ImVec2(120.0f, 0.0f))) {
            if (escenaExiste(st, ruta) && st.saveAsConfirm != ruta) {
                // Primer click sobre un archivo que ya existe: avisa y espera. El
                // segundo click llega con saveAsConfirm == ruta y si pisa.
                st.saveAsConfirm = ruta;
                st.saveAsError   = "Ya existe. Apreta Guardar de nuevo para pisarlo.";
            } else if (guardarEscenaComo(ctx, ruta)) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(120.0f, 0.0f))) ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

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
            // Rotulo, no accion: MenuItem con enabled=false para que se vea gris
            // y no se pueda clickear. Es la unica pista de que archivo se esta
            // editando, y de si hay archivo.
            ImGui::MenuItem(st.currentScenePath.empty()
                                ? "(sin guardar)"
                                : nombreDeArchivo(st.currentScenePath).c_str(),
                            nullptr, false, false);
            ImGui::Separator();

            // Load ya hace Clear() de grafo y assets: no hay que limpiar a mano.
            if (ImGui::BeginMenu("Cargar")) {
                st.catalog.EnsureScanned();
                const std::vector<std::string>& escenas = st.catalog.Scenes();

                if (escenas.empty()) {
                    ImGui::MenuItem("(no hay escenas)", nullptr, false, false);
                } else {
                    for (const std::string& ruta : escenas) {
                        const bool abierta = (ruta == st.currentScenePath);
                        if (ImGui::MenuItem(nombreDeArchivo(ruta).c_str(), nullptr, abierta)) {
                            cargarEscena(ctx, ruta);
                        }
                    }
                }
                ImGui::Separator();
                // El catalogo cachea: si copiaste un .efe con el sandbox abierto,
                // este es el boton que lo hace aparecer.
                if (ImGui::MenuItem("Refrescar lista")) st.catalog.Rescan();
                ImGui::EndMenu();
            }

            // Sin archivo (Cornell recien armada) no hay nada que sobrescribir:
            // el item queda gris y el camino es "Guardar como...".
            const bool tieneArchivo = !st.currentScenePath.empty();
            if (ImGui::MenuItem("Guardar", nullptr, false, tieneArchivo)) {
                serialization::SceneSerializer::Save(st.currentScenePath.c_str(), ctx.scene,
                                                     ctx.assets, ctx.rm, ctx.registry);
            }

            if (ImGui::MenuItem("Guardar como...")) {
                // Arranca con el nombre de la escena abierta: lo normal es derivar
                // una variante, no escribir un nombre de cero.
                st.saveAsName = st.currentScenePath.empty()
                                  ? std::string("nueva.efe")
                                  : nombreDeArchivo(st.currentScenePath);
                st.saveAsConfirm.clear();
                st.saveAsError.clear();
                // El catalogo tiene que estar fresco: es lo que responde si el
                // archivo ya existe.
                st.catalog.Rescan();
                st.openSaveAs = true;
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Sala de Cornell")) {
                BuildCornellScene(ctx);        // hace Clear de escena y assets, y RefreshHandles
                st.currentScenePath.clear();   // escena generada en codigo: no salio de ningun archivo
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
            // Mismo patron que "Animate": el item lleva el bool y el if corre en
            // el frame en que cambia.
            bool simulando = ctx.game != null && ctx.game->Simulating();
            if (ImGui::MenuItem("Simular", nullptr, &simulando, ctx.game != null)) {
                if (simulando) ctx.game->BeginSimulation();
                else           ctx.game->EndSimulation();
            }
            if (ctx.game == null) ImGui::TextDisabled("  sin fisica: Jolt no arranco");
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
            ImGui::MenuItem("Rendimiento",      nullptr, &st.showPerf);
            ImGui::MenuItem("Colliders",        nullptr, &st.showColliders);
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

            // La raiz no puede arrastrarse: no tiene padre que cambiar.
            // El payload es el NodeHandle crudo (indice + generacion): es un POD
            // y ImGui se queda con una copia de los bytes.
            if (h != ctx.scene.Root() && ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("EF_NODE", &h, sizeof(h));
                ImGui::Text("%s", node.name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                // Se espia el payload SIN aceptarlo: si el drop crearia un ciclo
                // (soltar sobre si mismo o sobre un descendiente) no se acepta, y
                // asi ImGui tampoco ilumina el target. Rechazarlo despues, dentro
                // del motor, dejaria al usuario soltando sin que pase nada.
                const ImGuiPayload* espia = ImGui::GetDragDropPayload();
                bool valido = false;
                if (espia != nullptr && espia->IsDataType("EF_NODE")) {
                    const scene::NodeHandle arrastrado =
                        *static_cast<const scene::NodeHandle*>(espia->Data);
                    valido = !ctx.scene.IsAncestorOrSelf(arrastrado, h);
                }

                if (valido) {
                    if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("EF_NODE")) {
                        st.pendingReparentChild  = *static_cast<const scene::NodeHandle*>(p->Data);
                        st.pendingReparentParent = h;
                    }
                }
                ImGui::EndDragDropTarget();
            }

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

        // Recien aca es seguro tocar la jerarquia: el recorrido ya termino.
        // Se revalidan los dos handles porque entre el drop y este punto podria
        // haberse destruido cualquiera de los dos.
        if (!st.pendingReparentChild.IsNull()) {
            if (ctx.scene.IsValid(st.pendingReparentChild) &&
                ctx.scene.IsValid(st.pendingReparentParent)) {
                ctx.scene.SetParentKeepWorld(st.pendingReparentChild, st.pendingReparentParent);
            }
            st.pendingReparentChild  = scene::NodeHandle{};
            st.pendingReparentParent = scene::NodeHandle{};
        }

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

        {
        CamposAlineados alineados;   // se cierra antes del End() de abajo

        ImGui::SeparatorText("Nodo");

        // Escribe directo sobre el nodo, sin confirmar y sin validar: el nombre
        // es cosmetico. La identidad real es el NodeHandle y el .efe guarda el
        // parent por indice, asi que un nombre vacio o repetido no rompe nada.
        // Lo unico que se degrada es FindByName, que ya devuelve el primer match
        // por contrato.
        ImGui::InputText("Nombre", &node.name);

        // El world es derivado: se muestra como lectura, no se edita.
        const glm::vec3 worldPos = glm::vec3(node.worldMatrix[3]);
        ImGui::TextDisabled("mundo   %.2f, %.2f, %.2f", worldPos.x, worldPos.y, worldPos.z);
        ImGui::TextDisabled("handle  index=%u, gen=%u", node.self.index, node.self.generation);

        if (ImGui::Button("Encuadrar (F)", ImVec2(-kAnchoEtiqueta, 0.0f))) FocusSelection(ctx);

        // "(local)" en el titulo y no en cada etiqueta: es la misma aclaracion
        // para los tres campos y repetirla los hacia tres veces mas largos.
        ImGui::SeparatorText("Transform (local)");
        math::Transform t = node.local;
        bool changed = false;
        changed |= ImGui::DragFloat3("Posicion", glm::value_ptr(t.position), 0.1f);
        changed |= ImGui::DragFloat3("Rotacion", glm::value_ptr(t.rotation), 0.5f);
        changed |= ImGui::DragFloat3("Escala",   glm::value_ptr(t.scale),    0.05f);
        if (changed) ctx.scene.SetLocalTransform(st.selected, t);

        DrawMeshSection(ctx, st.selected);

        ImGui::SeparatorText("Camara");
        if (node.camera) {
            scene::CameraAttachment& c = *node.camera;

            // Directo sobre el attachment: es data del nodo y no hay nada que
            // recalcular. Los limites son los que hacen singular la proyeccion
            // (un fov de 0 o de 180, un near en 0), no gusto personal.
            ImGui::DragFloat("FOV",         &c.fovDeg,    0.5f,  1.0f,  179.0f, "%.1f");
            ImGui::DragFloat("Near",        &c.nearPlane, 0.01f, 0.01f, 100.0f, "%.2f");
            ImGui::DragFloat("Far",         &c.farPlane,  5.0f,  1.0f,  20000.0f, "%.0f");
            ImGui::DragFloat("Exposicion",  &c.exposure,  0.01f, 0.0f,  5.0f,  "%.3f");
            if (c.farPlane <= c.nearPlane) c.farPlane = c.nearPlane + 1.0f;

            if (ctx.scene.ActiveCamera() == st.selected) {
                ImGui::TextDisabled("es la camara activa de la escena");
            } else if (ImGui::Button("Hacer activa", ImVec2(-kAnchoEtiqueta, 0.0f))) {
                ctx.scene.SetActiveCamera(st.selected);
            }

            if (ImGui::Button("Quitar camara", ImVec2(-kAnchoEtiqueta, 0.0f))) {
                ctx.scene.DetachCamera(st.selected);
            }
        } else if (ImGui::Button("Agregar camara", ImVec2(-kAnchoEtiqueta, 0.0f))) {
            ctx.scene.AttachCamera(st.selected, scene::CameraAttachment{});
        }

        ImGui::SeparatorText("Collider");
        if (node.collider) {
            scene::ColliderAttachment& col = *node.collider;

            const char* kFormas[] = { "Box", "Sphere", "Capsule", "Mesh" };
            int forma = static_cast<int>(col.kind);
            if (ImGui::Combo("Forma", &forma, kFormas, IM_ARRAYSIZE(kFormas))) {
                col.kind = static_cast<scene::ShapeKind>(forma);
            }

            // Cada forma lee params distinto: mostrar los tres siempre seria
            // pedirle al usuario que adivine cual importa.
            switch (col.kind) {
                case scene::ShapeKind::Box:
                    ImGui::DragFloat3("Semiejes", glm::value_ptr(col.params), 0.05f, 0.001f, 1000.0f);
                    break;
                case scene::ShapeKind::Sphere:
                    ImGui::DragFloat("Radio", &col.params.x, 0.05f, 0.001f, 1000.0f);
                    break;
                case scene::ShapeKind::Capsule:
                    ImGui::DragFloat("Radio",      &col.params.x, 0.05f, 0.001f, 1000.0f);
                    ImGui::DragFloat("Semialtura", &col.params.y, 0.05f, 0.001f, 1000.0f);
                    break;
                case scene::ShapeKind::Mesh:
                    ImGui::TextDisabled("la forma sale de la malla del nodo");
                    break;
            }

            ImGui::DragFloat3("Offset pos", glm::value_ptr(col.localOffset.position), 0.05f);
            ImGui::DragFloat3("Offset rot", glm::value_ptr(col.localOffset.rotation), 0.5f);

            const char* kMovimientos[] = { "Static", "Kinematic", "Dynamic" };
            int mov = static_cast<int>(col.motion);
            if (ImGui::Combo("Movimiento", &mov, kMovimientos, IM_ARRAYSIZE(kMovimientos))) {
                col.motion = static_cast<scene::MotionType>(mov);
            }

            // El campo ya viaja en el .efe, pero el binding todavia no lo
            // traduce: un checkbox que no hace nada es peor que uno apagado.
            ImGui::BeginDisabled(true);
            bool trigger = col.isTrigger;
            ImGui::Checkbox("Es trigger", &trigger);
            ImGui::EndDisabled();
            ImGui::TextDisabled("los triggers llegan en el ciclo 6");

            if (col.kind == scene::ShapeKind::Mesh && !node.mesh) {
                ImGui::TextDisabled("sin malla en el nodo: no va a tener cuerpo");
            }
            if (col.kind == scene::ShapeKind::Mesh && col.motion != scene::MotionType::Static) {
                ImGui::TextDisabled("una malla no puede moverse: se crea estatica");
            }
            if (ctx.game != null && ctx.game->Simulating()) {
                ImGui::TextDisabled("los cambios se aplican al reiniciar la simulacion");
            }

            if (ImGui::Button("Quitar collider", ImVec2(-kAnchoEtiqueta, 0.0f))) {
                ctx.scene.DetachCollider(st.selected);
            }
        } else if (ImGui::Button("Agregar collider", ImVec2(-kAnchoEtiqueta, 0.0f))) {
            ctx.scene.AttachCollider(st.selected, scene::ColliderAttachment{});
        }

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
        }

        ImGui::End();
    }



    void drawPerfPanel(EditorContext& ctx) {
        EditorState& st = ctx.state;

        if (!ImGui::Begin("Rendimiento", &st.showPerf)) { ImGui::End(); return; }

        const renderer::GpuProfiler& prof = ctx.app.GetProfiler();

        if (!prof.supported()) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f),
                               "El driver no mide marcas de tiempo: no hay datos de GPU.");
        }

        // -- Placa y driver --
        const efecom::DeviceInfo dev = efecom::GetDeviceInfo();
        ImGui::TextDisabled("%s  |  %s", dev.renderer, dev.apiVersion);
        ImGui::TextDisabled("%u x %u", ctx.app.GetWindow().GetWidth(),
                                       ctx.app.GetWindow().GetHeight());

        ImGui::Separator();

        // -- Contadores del frame --
        const efecom::FrameCounters c = efecom::GetFrameCounters();
        ImGui::Text("Draws: %u    Triangulos: %u    Dispatch: %u", c.drawCalls, c.triangles, c.dispatches);
        ImGui::Text("Cambios de estado: %u  (redundantes: %u)", c.stateApplies, c.stateRedundant);

        ImGui::Separator();

        // -- Vsync --
        bool vsync = ctx.app.GetWindow().IsVSync();
        if (ImGui::Checkbox("VSync", &vsync)) ctx.app.GetWindow().SetVSync(vsync);
        if (vsync) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
                               "(el frame esta clavado al monitor: apagalo para medir)");
        }

        // -- El propio medidor --
        // El profiler agrega ~24 llamadas de GL por frame. Es despreciable
        // frente a las miles que estamos investigando, pero tiene que poder
        // salir del medio: si un numero raro no cambia al apagarlo, no lo esta
        // causando el medidor.
        bool medir = prof.enabled();
        if (ImGui::Checkbox("Medir", &medir)) ctx.app.GetProfiler().SetEnabled(medir);

        ImGui::Separator();

        // -- La lectura principal: quien manda, CPU o GPU --
        const f32 gpu = prof.stats().TotalGpuMs();
        const f32 cpu = prof.stats().TotalCpuMs();
        ImGui::Text("GPU total: %6.3f ms", gpu);
        ImGui::Text("CPU total: %6.3f ms", cpu);
        if (cpu > gpu) {
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f),
                               "Manda la CPU: el cuello esta en ENVIAR el trabajo, no en dibujarlo.");
        } else {
            ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f),
                               "Manda la GPU: el cuello esta en el trabajo de la placa.");
        }

        ImGui::Separator();

        // -- La tabla, en ORDEN DE FRAME. Ordenarla por costo haria saltar las
        // -- filas entre corridas y volveria imposible comparar dos mediciones.
        if (ImGui::BeginTable("pases", 5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Pase");
            ImGui::TableSetupColumn("GPU ms");
            ImGui::TableSetupColumn("CPU ms");
            ImGui::TableSetupColumn("Draws");
            // El peor frame de la ventana, GPU o CPU, el que sea mas alto. Es la
            // columna que convierte "hay picos en el grafico de frametime" en
            // "los produce este pase".
            ImGui::TableSetupColumn("Peor ms");
            ImGui::TableHeadersRow();

            for (const renderer::PassRow& r : prof.stats().Rows()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                if (!r.present) {
                    // Un pase apagado muestra guiones, NO el ultimo valor que
                    // tuvo: arrastrarlo haria creer que sigue costando.
                    ImGui::TextDisabled("%s", r.name);
                    ImGui::TableNextColumn(); ImGui::TextDisabled("--");
                    ImGui::TableNextColumn(); ImGui::TextDisabled("--");
                    ImGui::TableNextColumn(); ImGui::TextDisabled("--");
                    ImGui::TableNextColumn(); ImGui::TextDisabled("--");
                    continue;
                }

                ImGui::Text("%s", r.name);
                ImGui::TableNextColumn(); ImGui::Text("%6.3f", r.gpuMs);
                ImGui::TableNextColumn(); ImGui::Text("%6.3f", r.cpuMs);
                ImGui::TableNextColumn(); ImGui::Text("%.0f",  r.drawCalls);

                // El peor frame de la ventana, en rojo si se despego del
                // promedio. Es el instrumento para atribuir un pico: un pase que
                // cuesta 0.15 ms casi siempre y 4 ms cada 200 frames promedia
                // 0.17 y en la columna de promedio no se distingue de uno parejo.
                const f32 peor = (r.gpuMaxMs > r.cpuMaxMs) ? r.gpuMaxMs : r.cpuMaxMs;
                const f32 medio = (r.gpuMs > r.cpuMs) ? r.gpuMs : r.cpuMs;
                ImGui::TableNextColumn();
                if (medio > 0.0f && peor > medio * 2.0f) {
                    ImGui::TextColored(kColorAviso, "%6.3f", peor);
                } else {
                    ImGui::Text("%6.3f", peor);
                }
            }
            ImGui::EndTable();
        }

        ImGui::Separator();

        // -- El historico que FrameStats ya lleva --
        const FrameStats& fs = st.stats;
        ImGui::Text("Frame: %.2f ms  (%.0f FPS)", fs.AvgMs(), fs.AvgFps());

        // La cola, no la media. Un frame de 4.3 ms de promedio con dos picos de
        // 15 se siente peor que uno de 6 parejo, y el promedio dice lo contrario.
        ImGui::Text("p95 %.2f   p99 %.2f   max %.2f ms", fs.P95Ms(), fs.P99Ms(), fs.MaxMs());

        const u32 picos = fs.Spikes();
        if (picos > 0u) {
            ImGui::TextColored(kColorAviso,
                               "%u de %d frames por encima de %.0fx el promedio",
                               picos, fs.HistoryCount(), FrameStats::kSpikeFactor);
            ImGui::TextDisabled("mira la columna 'Peor ms' para ver que pase los produce");
        } else {
            ImGui::TextColored(kColorOk, "sin picos en los ultimos %d frames", fs.HistoryCount());
        }

        ImGui::PlotLines("##frame", fs.History(), fs.HistoryCount(), fs.HistoryOffset(),
                         nullptr, 0.0f, FLT_MAX, ImVec2(0.0f, 60.0f));

        ImGui::End();
    }

    void drawRenderPanel(EditorContext& ctx) {
        EditorState& st = ctx.state;

        if (!ImGui::Begin("Render", &st.showRender)) { ImGui::End(); return; }

        {
        CamposAlineados alineados;   // se cierra antes del End() de abajo

        if (ImGui::CollapsingHeader("Iluminacion", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Intensidad IBL", &ctx.scene.iblIntensity, 0.0f, 2.0f);
            // La exposicion tiene dos fuentes: el attachment de la camara activa
            // cuando manda la vista previa, y este valor cuando manda la freecam.
            // Con la vista previa prendida el slider se deshabilita en vez de
            // mentir: escribirlo no cambiaria nada, el attachment lo pisa.
            ImGui::BeginDisabled(st.previewCamera);
            ImGui::SliderFloat("Exposure", &st.editorCam.exposure, 0.0f, 5.0f);
            ImGui::EndDisabled();
            if (st.previewCamera) {
                ImGui::TextDisabled("manda la exposicion de la camara de la escena (%.3f)",
                                    ctx.camera.Exposure());
            }
        }

        // Los paneles de pases viven en panels/, uno por pase, y se
        // registran en PanelesDePases(). Este archivo no nombra ninguno.
        for (const DrawPanelFn draw : PanelesDePases()) draw(ctx);

        // -- Post ---------------------------------------------------------------
        // Bloom y FXAA corren sobre la imagen ya resuelta, asi que van despues de
        // todo lo que la produce. Nombres de campo en castellano como el resto
        // del panel: eran los unicos en ingles.
        if (ImGui::CollapsingHeader("Bloom")) {
            renderer::BloomSettings& s = ctx.app.GetBloomPass().settings();
            ImGui::SliderFloat("Umbral",      &s.threshold,  0.0f, 5.0f);
            ImGui::SliderFloat("Knee",        &s.knee,       0.0f, 1.0f);
            ImGui::SliderFloat("Intensidad",  &s.intensity,  0.0f, 0.5f);
            ImGui::SliderInt  ("Iteraciones", &s.iterations, 1,    10);
        }

        if (ImGui::CollapsingHeader("FXAA")) {
            renderer::FxaaSettings& fx = ctx.app.GetFxaaPass().settings();
            ImGui::Checkbox("Habilitado", &fx.enabled);
        }

        // -- Camara -------------------------------------------------------------
        // Ultima a proposito: no produce imagen, es como se navega, y una vez
        // configurada no se vuelve a tocar.
        if (ImGui::CollapsingHeader("Camara")) {
            ImGui::Checkbox("Vista previa de la escena", &st.previewCamera);
            const scene::NodeHandle activa = ctx.scene.ActiveCamera();
            if (st.previewCamera && !ctx.scene.IsValid(activa)) {
                ImGui::TextDisabled("la escena no tiene camara activa: manda la freecam");
            } else if (ctx.scene.IsValid(activa)) {
                ImGui::TextDisabled("camara activa: %s", ctx.scene.Get(activa).name.c_str());
            }
            ImGui::Separator();

            scene::CameraSettings& cs = ctx.controller.settings();
            ImGui::SliderFloat("Velocidad",     &cs.moveSpeed,       1.0f,    200.0f);
            ImGui::SliderFloat("Boost (Shift)", &cs.boostMultiplier, 1.0f,    20.0f);
            ImGui::SliderFloat("Sensibilidad",  &cs.lookSensitivity, 0.0005f, 0.02f,  "%.4f");
            ImGui::SliderFloat("Paneo",         &cs.panSpeed,        0.0001f, 0.01f,  "%.4f");
            ImGui::SliderFloat("Dolly",         &cs.dollySpeed,      0.01f,   0.5f);
            ImGui::Checkbox("Invertir X", &cs.invertX);
            ImGui::SameLine();
            ImGui::Checkbox("Invertir Y", &cs.invertY);

            ImGui::SeparatorText("Estado");
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
        }

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
            // El p99 al lado del promedio y no escondido en el panel grande: es
            // el numero que dice si la imagen se siente pareja, y si solo se ve
            // el promedio uno optimiza la media y empeora la cola sin enterarse.
            if (fs.Spikes() > 0u) {
                ImGui::TextColored(kColorAviso, "p99 %.2f ms   %u picos", fs.P99Ms(), fs.Spikes());
            } else {
                ImGui::TextDisabled("p99 %.2f ms", fs.P99Ms());
            }

            // Escala fija 0..33.3 ms (o sea, hasta 30 FPS). Autoescalada, el ruido
            // de un frame quieto se veria como una montana rusa y no informaria nada.
            ImGui::PlotLines("##ms", fs.History(), fs.HistoryCount(), fs.HistoryOffset(),
                             nullptr, 0.0f, 33.3f, ImVec2(220.0f, 40.0f));

            ImGui::Separator();
            const glm::vec3& p = ctx.camera.Position();
            ImGui::Text("cam  %.2f, %.2f, %.2f", p.x, p.y, p.z);
            ImGui::Text("exp  %.2f   asp %.2f", ctx.camera.Exposure(), ctx.app.GetWindow().GetAspectRatio());

            const core::Time& time = ctx.app.GetTime();
            ImGui::Text("fix  %d pasos   alpha %.2f%s",
                        time.FixedSteps(), time.Alpha(),
                        time.FixedStepsSaturated() ? "   SATURADO" : "");
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
    // Antes del early return de showUI: la barra de menu se dibuja siempre, asi
    // que el modal que abre tambien tiene que poder dibujarse siempre.
    drawSaveAsModal(ctx);

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
    if (ctx.state.showPerf)      drawPerfPanel(ctx);

    DrawSunGizmo(ctx, dockId);
    DrawColliderGizmos(ctx, dockId);
}

} // namespace sandbox
