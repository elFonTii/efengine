#include "EditorUI.h"

#include "AuthoringUI.h"
#include "TestScene.h"
#include "SunGizmo.h"

#include <efengine/application/Application.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/BloomPass.h>
#include <efengine/renderer/FxaaPass.h>
#include <efengine/renderer/ShadowPass.h>
#include <efengine/renderer/DdgiPass.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/DdgiVolume.h>
#include <efengine/renderer/AoPass.h>
#include <efengine/renderer/AoSettings.h>
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

    // Colores de mensaje de la UI. AuthoringUI.cpp tiene su propia copia de
    // kColorError en su namespace anonimo; son dos unidades de traduccion distintas.
    const ImVec4 kColorError { 1.00f, 0.40f, 0.40f, 1.0f };
    const ImVec4 kColorAviso { 1.00f, 0.80f, 0.30f, 1.0f };
    const ImVec4 kColorOk    { 0.45f, 0.85f, 0.45f, 1.0f };

    // Ancho reservado para las etiquetas de los campos. Los widgets se estiran
    // hasta el borde del panel menos esto, asi que todos empiezan y terminan
    // alineados en vez de tener cada uno el largo que le toco.
    constexpr f32 kAnchoEtiqueta = 150.0f;

    // RAII sobre PushItemWidth: se abre una al principio de cada panel y todo lo
    // que se dibuje adentro queda alineado sin repetir la llamada por widget.
    //
    // OJO: la pila de item width es POR VENTANA, asi que el guard tiene que morir
    // ANTES del ImGui::End() de su panel. Si vive hasta el final de la funcion, el
    // Pop cae en la ventana de afuera y ImGui asserta con "Calling PopItemWidth()
    // too many times!". Por eso los paneles lo meten en un bloque propio.
    struct CamposAlineados {
        explicit CamposAlineados(f32 anchoEtiqueta = kAnchoEtiqueta) {
            ImGui::PushItemWidth(-anchoEtiqueta);
        }
        ~CamposAlineados() { ImGui::PopItemWidth(); }

        CamposAlineados(const CamposAlineados&)            = delete;
        CamposAlineados& operator=(const CamposAlineados&) = delete;
    };

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

    void drawDdgiSection(EditorContext& ctx) {
        if (!ImGui::CollapsingHeader("DDGI (iluminacion indirecta)")) return;

        std::optional<renderer::DdgiPass>& opt = ctx.app.GetDdgiPass();
        if (!opt.has_value()) {
            ImGui::TextColored(kColorError, "DdgiPass no disponible: fallo la carga de shaders.");
            ImGui::TextWrapped("La escena esta usando IBL puro. Mira la consola.");
            return;
        }
        renderer::DdgiPass&     pass = *opt;
        renderer::DdgiSettings& s    = pass.settings();

        CamposAlineados alineados;

        // -- Lo primero que hay que mirar cuando "no se ve la GI" --------------
        if (pass.atlasValid()) {
            ImGui::TextColored(kColorOk, "pbr.frag recibe los atlas: SI");
        } else {
            ImGui::TextColored(kColorError, "pbr.frag recibe los atlas: NO");
            ImGui::TextWrapped("Hasta que corra un blend, DDGI aporta cero y la imagen es IBL puro.");
        }

        ImGui::Checkbox("Habilitado", &s.enabled);

        // -- Grilla ------------------------------------------------------------
        ImGui::SeparatorText("Grilla");
        bool gridChanged = false;
        gridChanged |= ImGui::DragFloat3("Origen",         &s.grid.origin.x,  0.1f);
        gridChanged |= ImGui::DragFloat3("Espaciado",      &s.grid.spacing.x, 0.05f, 0.05f, 10.0f);
        gridChanged |= ImGui::DragInt3  ("Probes por eje", &s.grid.counts.x,  1.0f,
                                         1, renderer::kMaxProbesPerAxis);
        ImGui::TextDisabled("total: %u probes", renderer::ProbeCount(s.grid));

        // Encajar la grilla a la escena resuelve de un click la clase entera de
        // bug "la grilla no cubre la sala", que es con la que arranco este ciclo.
        if (ImGui::Button("Encajar grilla a la escena", ImVec2(-kAnchoEtiqueta, 0.0f))) {
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
            ImGui::TextColored(kColorAviso,
                               "Cambiar la grilla realoca los atlas y reinicia el barrido.");
        }

        // -- Update ------------------------------------------------------------
        ImGui::SeparatorText("Update");
        int perFrame = static_cast<int>(s.probesPerFrame);
        if (ImGui::SliderInt("Probes por frame", &perFrame, 0,
                             static_cast<int>(renderer::kMaxProbesPerFrame))) {
            s.probesPerFrame = static_cast<u32>(perFrame);
        }
        // Histeresis: cuanto del valor viejo se conserva. ESTO es el denoise
        // temporal de DDGI, no hace falta un denoiser aparte. Mas alto = mas
        // estable y mas lento en reaccionar.
        ImGui::SliderFloat("Histeresis", &s.hysteresis, 0.0f, 0.995f, "%.3f");
        ImGui::Checkbox("Congelar (freeze)", &s.freeze);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) pass.Reset();

        const u32 total = renderer::ProbeCount(s.grid);
        const u32 framesPorBarrido = (s.probesPerFrame > 0u)
                                   ? (total + s.probesPerFrame - 1u) / s.probesPerFrame
                                   : 0u;
        ImGui::TextDisabled("cursor %u / %u   barridos %u", pass.cursor(), total, pass.sweepsDone());
        ImGui::TextDisabled("frames por barrido: %u", framesPorBarrido);
        // Tiempo de CPU emitiendo las llamadas, no de GPU ejecutandolas: sirve
        // para detectar que el round-robin se fue de escala, no como profiler.
        ImGui::TextDisabled("pase (CPU): %.3f ms", pass.lastMs());

        // -- Sampleo -----------------------------------------------------------
        ImGui::SeparatorText("Sampleo");
        ImGui::SliderFloat("Intensidad", &s.intensity, 0.0f, 4.0f);
        // Normal bias: subir si la luz atraviesa las paredes; bajar si los
        // rincones tienen una banda oscura.
        ImGui::SliderFloat("Normal bias", &s.normalBias, 0.0f, 1.0f, "%.3f m");
        ImGui::SliderFloat("View bias", &s.viewBias, 0.0f, 1.0f, "%.3f m");
        ImGui::SliderFloat("Chebyshev", &s.chebyshevSharpness, 1.0f, 16.0f);

        // El rango sale de la escena, no de un numero fijo: con un tope de 100 m
        // fijo, abrir el panel con maxDistance en 200 lo clamparia en silencio y
        // cambiaria el far plane de la captura sin que nadie toque nada.
        const renderer::AABB& bounds = ctx.scene.WorldBounds();
        const f32 topeDist = bounds.Valid() ? glm::max(4.0f * bounds.Radius(), 10.0f) : 200.0f;
        // Es el far plane de la captura de probes: muy alto tira la precision
        // del depth, muy bajo deja la captura vacia.
        ImGui::SliderFloat("Distancia max", &s.maxDistance, 1.0f, topeDist, "%.1f m");

        // -- Debug -------------------------------------------------------------
        ImGui::SeparatorText("Debug");

        // Primero de la seccion a proposito: es el unico control que contesta la
        // pregunta con la que uno abre este panel, "DDGI esta aportando algo".
        // El orden espeja DdgiSettings::DebugView.
        const char* vistas[] = { "Final (normal)",
                                 "Indirecta (irradiancia)",
                                 "Indirecta aplicada al pixel",
                                 "Solo luz directa",
                                 "DDGI ignorando el fade",
                                 "Fade del volumen",
                                 "Albedo",
                                 "Normal",
                                 "Sombra del sol (cruda)" };
        //
        // Como se leen estas vistas (era un tooltip; vive aca para no tapar la UI):
        //
        //   Eligen que termino escribe pbr.frag en vez de la imagen final.
        //
        //   'Indirecta aplicada' es lo que la indirecta le suma al pixel. OJO: ahi
        //   adentro el IBL y DDGI van MEZCLADOS por el fade, asi que no ver negro no
        //   prueba que DDGI aporte. Para leerlo, primero Render > Iluminacion >
        //   Intensidad IBL a 0: lo que quede es DDGI.
        //
        //   Y para saber por que: comparar 'DDGI ignorando el fade' con 'Fade del
        //   volumen'. Si el primero tiene color y el segundo esta negro, la GI se
        //   calcula bien y la tira el fade -- la grilla no cubre esa superficie con
        //   el margen que el fade pide.
        //
        //   Todas pasan por bloom y ACES: son cualitativas, no numeros.
        //
        int vista = static_cast<int>(s.debugView);
        if (ImGui::Combo("Vista", &vista, vistas, IM_ARRAYSIZE(vistas))) {
            s.debugView = static_cast<u32>(vista);
        }
        if (s.debugView != renderer::DdgiSettings::kDebugOff) {
            ImGui::TextColored(kColorAviso, "Vista de debug activa: la imagen NO es la final.");
        }

        ImGui::Checkbox("Mostrar probes", &s.debugProbes);
        const char* modos[] = { "Irradiancia", "Media de distancia", "Target de captura",
                                "Target de captura (distancia)" };
        int modo = static_cast<int>(s.debugMode);
        if (ImGui::Combo("Modo", &modo, modos, 4)) s.debugMode = static_cast<u32>(modo);
        ImGui::SliderFloat("Radio de esfera", &s.debugRadius, 0.02f, 0.5f, "%.3f m");
    }

    void drawAoSection(EditorContext& ctx) {
        if (!ImGui::CollapsingHeader("Oclusion ambiental (GTAO)")) return;

        std::optional<renderer::AoPass>& opt = ctx.app.GetAoPass();
        if (!opt.has_value()) {
            ImGui::TextColored(kColorError, "AoPass no disponible: fallo la carga de shaders.");
            ImGui::TextWrapped("La escena esta sin oclusion de contacto. Mira la consola.");
            return;
        }
        renderer::AoSettings& s = opt->settings();

        CamposAlineados alineados;

        ImGui::Checkbox("Habilitado", &s.enabled);

        ImGui::SeparatorText("Trazado");
        // El radio va en METROS y tiene que quedar POR DEBAJO del espaciado de
        // probes de DDGI: lo que el AO ocluye es exactamente lo que la grilla no
        // puede ver. Por encima, los dos oscurecen la misma cosa. Radio 0 =
        // control nulo, la imagen tiene que volver a ser la de AO apagado.
        ImGui::SliderFloat("Radio", &s.radius, 0.0f, 3.0f, "%.2f m");

        // El espaciado de DDGI al lado del slider: la regla "radio < espaciado"
        // no se puede verificar de otra forma desde el panel.
        std::optional<renderer::DdgiPass>& ddgi = ctx.app.GetDdgiPass();
        if (ddgi.has_value()) {
            const glm::vec3& sp = ddgi->settings().grid.spacing;
            const f32 minSp = glm::min(sp.x, glm::min(sp.y, sp.z));
            if (s.radius >= minSp) {
                ImGui::TextColored(kColorAviso,
                                   "Radio >= espaciado de probes (%.2f m): doble oscurecimiento.", minSp);
            } else {
                ImGui::TextDisabled("espaciado de probes mas chico: %.2f m", minSp);
            }
        }

        ImGui::SliderFloat("Intensidad", &s.intensity, 0.0f, 4.0f);
        // Grosor: que tan rapido se desvanece una muestra lejana. Bajarlo hace
        // que un objeto lejano alineado en pantalla deje de ocluir a uno cercano.
        ImGui::SliderFloat("Grosor",     &s.thickness, 0.01f, 1.0f);
        ImGui::SliderInt  ("Cortes",     &s.slices, 1, 8);
        ImGui::SliderInt  ("Pasos",      &s.steps,  1, 32);
        ImGui::SliderFloat("Techo de radio", &s.maxScreenRadius, 8.0f, 512.0f, "%.0f px");

        ImGui::SeparatorText("Aplicacion");
        // Bent normal: orienta el lookup de irradiancia (IBL y DDGI) hacia donde
        // el hemisferio esta abierto. En un rincon de Cornell cambia la DIRECCION
        // del color bleeding.
        ImGui::Checkbox("Bent normal",  &s.bentNormal);
        // Multi-rebote: el AO se tiñe con el albedo en vez de oscurecer a gris.
        // Sobre la pared roja la diferencia es directa.
        ImGui::Checkbox("Multi-rebote", &s.multiBounce);
        ImGui::Checkbox("Blur", &s.blur);

        ImGui::SeparatorText("Debug");
        const char* vistas[] = { "Final (normal)",
                                 "Visibilidad",
                                 "Bent normal (world)",
                                 "Normal del prepass (view)",
                                 "Profundidad (1 banda = 1 m)" };
        // Las dos ultimas vuelcan el prepass y saltean el blur. Si esta vista y
        // la de DDGI estan las dos activas, gana esta.
        int vista = static_cast<int>(s.debugView);
        if (ImGui::Combo("Vista AO", &vista, vistas, IM_ARRAYSIZE(vistas))) {
            s.debugView = static_cast<u32>(vista);
        }
        if (s.debugView != 0u) {
            ImGui::TextColored(kColorAviso, "Vista de debug activa: la imagen NO es la final.");
        }
    }

    void drawRenderPanel(EditorContext& ctx) {
        EditorState& st = ctx.state;

        if (!ImGui::Begin("Render", &st.showRender)) { ImGui::End(); return; }

        {
        CamposAlineados alineados;   // se cierra antes del End() de abajo

        if (ImGui::CollapsingHeader("Iluminacion", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Intensidad IBL", &ctx.scene.iblIntensity, 0.0f, 2.0f);
            f32 exposure = ctx.camera.Exposure();
            if (ImGui::SliderFloat("Exposure", &exposure, 0.0f, 5.0f)) ctx.camera.SetExposure(exposure);
        }

        if (ImGui::CollapsingHeader("Sombras", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderer::ShadowPass&     pase = ctx.app.GetShadowPass();
            renderer::ShadowSettings& sh   = pase.settings();

            ImGui::Checkbox   ("Habilitadas", &sh.enabled);
            // Margen: aire alrededor de la escena. El encuadre de la luz sale de
            // sus bounds y esto es lo unico a mano. Mas margen = texel mas
            // grande = mas acne.
            ImGui::SliderFloat("Margen",      &sh.padding, 0.0f, 10.0f, "%.2f m");
            // Sirve para medir: si un artefacto se afina a la mitad al duplicar
            // la resolucion, escala con el texel y es del shadow map.
            const char* resoluciones[] = { "512", "1024", "2048", "4096" };
            const u32   valores[]      = { 512u,  1024u,  2048u,  4096u  };
            int resSel = 2;
            for (int i = 0; i < IM_ARRAYSIZE(valores); ++i)
                if (valores[i] == sh.resolution) resSel = i;
            if (ImGui::Combo("Resolucion", &resSel, resoluciones, IM_ARRAYSIZE(resoluciones))) {
                sh.resolution = valores[resSel];
            }

            // El mecanismo principal contra el acne (lineas oscuras en zonas
            // iluminadas): subirlo. NO abre luz en los rincones; el techo es la
            // geometria fina, que empieza a filtrar.
            ImGui::SliderFloat("Normal offset", &sh.normalOffsetTexels, 0.0f, 8.0f, "%.1f texels");

            // Los dos bias son la escotilla: empujan la profundidad hacia la luz,
            // asi que cualquier valor > 0 abre una banda de luz en las aristas
            // entre paredes. Dejarlos en 0.
            ImGui::SliderFloat("Bias min", &sh.biasMin, 0.0f, 0.01f, "%.4f");
            ImGui::SliderFloat("Bias max", &sh.biasMax, 0.0f, 0.02f, "%.4f");

            // Los dos bias son fracciones de profundidad NDC, que no quiere
            // decir nada solo. Lo que se tunea de verdad es cuantos texels de
            // holgura son, asi que se muestra la conversion.
            const renderer::DirectionalLightFit& fit = pase.fit();
            const f32 texel = (pase.resolution() > 0)
                            ? 2.0f * fit.orthoHalfSize / static_cast<f32>(pase.resolution())
                            : 0.0f;
            ImGui::TextDisabled("Encuadre: half %.1f m | rango %.1f m | texel %.1f mm",
                                fit.orthoHalfSize, fit.depthRange, texel * 1000.0f);
            ImGui::TextDisabled("Normal offset = %.1f mm | Bias Max = %.1f mm",
                                sh.normalOffsetTexels * texel * 1000.0f,
                                sh.biasMax * fit.depthRange * 1000.0f);
        }

        drawDdgiSection(ctx);
        drawAoSection(ctx);

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

    DrawSunGizmo(ctx, dockId);
}

} // namespace sandbox
