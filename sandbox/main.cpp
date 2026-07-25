#include <efengine/application/Application.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/Vertex.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/DirectionalLight.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/resources/MaterialBuilder.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/serialization/SceneSerializer.h>
#include <efengine/serialization/SceneRegistry.h>
#include <efengine/serialization/BinaryReader.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/CameraController.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/Node.h>
#include <efengine/scene/Behavior.h>
#include <efengine/math/Transform.h>
#include <efengine/core/Types.h>
#include <efengine/core/Log.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>

#include <imgui.h>
#include <cstdio>
#include <cstring>
#include <typeinfo>

#include <optional>
#include <string>
#include <memory>
#include <cmath>
#include <unordered_map>
#include <functional>
#include <vector>

namespace {
    using namespace efengine;

    // Solo compone paths: no toca el ResourceManager, asi que no puede fallar.
    // El def es lo que se guarda en el .efe; BuildMaterial lo convierte en runtime.
    renderer::MaterialDef makePbrMaterialDef(
            const std::string& name, const std::string& base,
            const std::string& res, const std::string& ext, bool withHeight) {
        renderer::MaterialDef def;
        def.name       = name;
        def.shaderName = "pbr";
        def.vertPath   = "assets/shaders/pbr.vert";
        def.fragPath   = "assets/shaders/pbr.frag";

        auto add = [&](renderer::TextureSlot slot, const char* map, renderer::ColorSpace space) {
            def.textures.push_back(renderer::TextureDef{
                slot, base + map + "_" + res + ext, space });
        };

        add(renderer::TextureSlot::Albedo,    "diff",   renderer::ColorSpace::sRGB);
        add(renderer::TextureSlot::Normal,    "nor_gl", renderer::ColorSpace::Linear);
        add(renderer::TextureSlot::Roughness, "rough",  renderer::ColorSpace::Linear);
        add(renderer::TextureSlot::AO,        "ao",     renderer::ColorSpace::Linear);
        if (withHeight) add(renderer::TextureSlot::Height, "disp", renderer::ColorSpace::Linear);

        return def;
    }

    // Params del generador de plano. Es lo que viaja en el .efe como genPayload.
    struct PlaneParams {
        f32 halfSize = 300.0f;
        f32 tiles    = 24.0f;

        template <class Ar>
        void Serialize(Ar& ar) {
            ar.Field(halfSize);
            ar.Field(tiles);
        }
    };

    // El submesh se llama siempre "ground": ese nombre es la clave del binding de
    // material en el archivo, asi que no puede variar.
    renderer::Model makePlane(f32 halfSize, f32 tiles) {
        const std::vector<renderer::Vertex> vertices = {
            // position                            normal         uv                tangent
            { {-halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f,  0.0f},  {1.0f, 0.0f, 0.0f} },
            { { halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {tiles, 0.0f},  {1.0f, 0.0f, 0.0f} },
            { { halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {tiles, tiles}, {1.0f, 0.0f, 0.0f} },
            { {-halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f,  tiles}, {1.0f, 0.0f, 0.0f} },
        };
        const std::vector<u32> indices = { 0, 3, 2, 2, 1, 0 }; // CCW visto desde +Y: winding concuerda con la normal (0,1,0)

        std::vector<renderer::Mesh> meshes;
        meshes.emplace_back(vertices, indices, "ground");
        return renderer::Model(std::move(meshes));
    }

    // Generador registrable: lee sus params del payload y devuelve el modelo.
    std::unique_ptr<renderer::Model> generarPlano(serialization::BinaryReader& r) {
        PlaneParams p;
        p.Serialize(r);
        if (!r.Ok()) return nullptr;
        return std::make_unique<renderer::Model>(makePlane(p.halfSize, p.tiles));
    }

    // BEHAVIORS
    // Los tres necesitan constructor por defecto (lo exige el registry) y un Serialize
    // miembro que describa sus campos una sola vez, para ambas direcciones.
    class RotarY : public scene::Behavior {
        public:
            RotarY() = default;
            explicit RotarY(f32 degPerSec) : m_degPerSec(degPerSec) {}

            void OnUpdate(scene::UpdateContext& ctx) override {
                math::Transform t = ctx.node.local;
                t.rotation.y += ctx.dt * m_degPerSec;
                ctx.SetLocal(t);
            }

            template <class Ar>
            void Serialize(Ar& ar) { ar.Field(m_degPerSec); }

        private:
            f32 m_degPerSec = 0.0f;
    };

    class OrbitarXZ : public scene::Behavior {
        public:
            OrbitarXZ() = default;
            OrbitarXZ(glm::vec3 center, f32 radius, f32 speed)
                : m_center(center), m_radius(radius), m_speed(speed) {}

            void OnUpdate(scene::UpdateContext& ctx) override {
                m_angle += ctx.dt * m_speed;
                math::Transform t = ctx.node.local;
                t.position = m_center + m_radius * glm::vec3(std::cos(m_angle), 0.0f, std::sin(m_angle));
                ctx.SetLocal(t);
            }

            template <class Ar>
            void Serialize(Ar& ar) {
                ar.Field(m_center);
                ar.Field(m_radius);
                ar.Field(m_speed);
                ar.Field(m_angle);   // se guarda: la orbita reanuda donde estaba
            }

        private:
            glm::vec3 m_center { 0.0f };
            f32 m_radius = 0.0f;
            f32 m_speed  = 0.0f;
            f32 m_angle  = 0.0f;
    };

    class RotarSolY : public scene::Behavior {
        public:
            RotarSolY() = default;
            explicit RotarSolY(f32 degPerSec) : m_degPerSec(degPerSec) {}

            void OnUpdate(scene::UpdateContext& ctx) override {
                m_angle += ctx.dt * m_degPerSec;
                math::Transform t = ctx.node.local;
                t.rotation.y = m_angle;
                ctx.SetLocal(t);
            }

            template <class Ar>
            void Serialize(Ar& ar) {
                ar.Field(m_degPerSec);
                ar.Field(m_angle);
            }

        private:
            f32 m_degPerSec = 0.0f;
            f32 m_angle     = 0.0f;
    };
}

int main() {
    using namespace efengine;

    EF_LOG_INFO("=== efengine: sandbox street rat ===");

    application::Application app;
    app.SetClearColor(0.18f, 0.18f, 0.18f);
    resources::ResourceManager& rm = app.GetResources();

    // Lo que el serializador tiene que saber de los tipos del sandbox.
    serialization::SceneRegistry registry;
    registry.behaviors.Register<RotarY>("RotarY");
    registry.behaviors.Register<OrbitarXZ>("OrbitarXZ");
    registry.behaviors.Register<RotarSolY>("RotarSolY");
    registry.meshes.Register("sandbox.plane", &generarPlano);

    // Dueno de los materiales y de las mallas generadas de la escena.
    resources::SceneAssets assets;

    renderer::Model* rat  = rm.GetModel("assets/models/street_rat_4k.fbx");
    renderer::Model* lamp = rm.GetModel("assets/models/industrial_pipe_lamp_2k.fbx");

    renderer::MaterialDef ratDef    = makePbrMaterialDef(
        "street_rat", "assets/textures/street_rat/street_rat_", "4k", ".jpg", false);
    renderer::MaterialDef groundDef = makePbrMaterialDef(
        "ground", "assets/textures/brown_mud/brown_mud_03_", "2k", ".jpg", true);
    renderer::MaterialDef lampDef   = makePbrMaterialDef(
        "industrial_lamp", "assets/textures/industrial_lamp/industrial_pipe_lamp_", "2k", ".jpg", true);

    // Los tweaks que antes se hacian sobre el Material ahora van en el def: son lo que
    // se guarda en el archivo.
    groundDef.heightScale = 0.0f;
    lampDef.heightScale   = 0.0f;
    lampDef.metallic      = 0.8f;
    lampDef.roughness     = 1.0f;

    auto adoptar = [&](const renderer::MaterialDef& def) -> u32 {
        std::optional<renderer::Material> m = resources::BuildMaterial(def, rm);
        if (!m) return resources::SceneAssets::kInvalidIndex;
        return assets.AddMaterial(def, std::move(*m));
    };

    const u32 iRat    = adoptar(ratDef);
    const u32 iGround = adoptar(groundDef);
    const u32 iLamp   = adoptar(lampDef);

    if (!rat || !lamp
        || iRat    == resources::SceneAssets::kInvalidIndex
        || iGround == resources::SceneAssets::kInvalidIndex
        || iLamp   == resources::SceneAssets::kInvalidIndex) {
        EF_LOG_ERROR("No se pudieron cargar los recursos");
        return 1;
    }

    // El plano se crea por el generador, para que quede registrado con su nombre y
    // payload y el .efe lo pueda recrear.
    std::vector<u8> planoPayload;
    std::unique_ptr<renderer::Model> planoModel =
        serialization::CreateGenerated(registry.meshes, "sandbox.plane", PlaneParams{}, planoPayload);
    if (!planoModel) {
        EF_LOG_ERROR("No se pudo generar el plano");
        return 1;
    }
    const u32 iPlano = assets.AddGenerated("sandbox.plane", planoPayload, std::move(planoModel));

    renderer::MaterialMap ratMats = {
        { "street_rat",      assets.MaterialAt(iRat) },
        { "street_rat_hair", assets.MaterialAt(iRat) },
    };

    renderer::MaterialMap lampMats;
    for (const renderer::Mesh& mesh : lamp->meshes()) {
        lampMats[mesh.materialName()] = assets.MaterialAt(iLamp);
    }

    renderer::MaterialMap groundMats = {
        { "ground", assets.MaterialAt(iGround) },
    };

    scene::SceneGraph scene;
    scene.ambientFactor = 0.08f;

    math::Transform ratTransform;
    ratTransform.scale = glm::vec3(10.0f);
    const scene::NodeHandle ratHandle = scene.CreateNode("model_rata");
    scene.AttachMesh(ratHandle, { rat, ratMats });
    scene.SetLocalTransform(ratHandle, ratTransform);

    const scene::NodeHandle groundNode = scene.CreateNode("plano");
    scene.AttachMesh(groundNode, { assets.GeneratedAt(iPlano), groundMats });

    math::Transform lampTransform;
    lampTransform.position = glm::vec3(30.0f, 0.0f, 0.0f);
    const scene::NodeHandle lampNode = scene.CreateNode("model_lampara");
    scene.AttachMesh(lampNode, { lamp, lampMats });
    scene.SetLocalTransform(lampNode, lampTransform);

    auto makePointLight = [&](const char* name, glm::vec3 pos, glm::vec3 color) {
        scene::NodeHandle n = scene.CreateNode(name);
        math::Transform t; t.position = pos;
        scene.SetLocalTransform(n, t);
        scene.AttachLight(n, { scene::LightKind::Point, color });
        return n;
    };
    const scene::NodeHandle orbitLight = makePointLight("luz_animada", glm::vec3(50.0f, 80.0f, 0.0f), glm::vec3(5000.0f));
    makePointLight("luz_2", glm::vec3(-50.0f, 80.0f, 0.0f), glm::vec3(5000.0f));
    makePointLight("luz_3", glm::vec3(  0.0f, 90.0f, 0.0f), glm::vec3(5000.0f));


    const scene::NodeHandle sunNode = scene.CreateNode("directional_light");
    math::Transform sunT; sunT.rotation = glm::vec3(-70.15f, 56.3f, 0.0f);
    scene.SetLocalTransform(sunNode, sunT);
    scene.AttachLight(sunNode, { scene::LightKind::Directional, glm::vec3(3.0f) });
    scene.SetPrimarySun(sunNode);

    bool animate = true;
    bool animateSun = false;

    scene.AttachBehavior(ratHandle,  std::make_unique<RotarY>(20.0f))->enabled = animate;
    scene.AttachBehavior(orbitLight, std::make_unique<OrbitarXZ>(glm::vec3(0.0f, 80.0f, 0.0f), 50.0f, 1.0f))->enabled = animate;
    scene.AttachBehavior(sunNode,    std::make_unique<RotarSolY>(30.0f))->enabled = animateSun;

    // Prende/apaga todos los behaviors de node
    auto setEnabled = [&](scene::NodeHandle h, bool on) {
        if (scene.IsValid(h))
            for (std::unique_ptr<scene::Behavior>& b : scene.Get(h).behaviors) b->enabled = on;
    };

    scene::Camera cam;
    cam.SetAspect(app.GetWindow().GetAspectRatio());
    scene::CameraController controller(&cam);
    app.GetWindow().SetEventListener(&controller);

    scene::NodeHandle selected;
    u32 spawnCounter = 0;


    while (app.Running()) {
        app.BeginFrame();
        if (app.IsKeyPressed(platform::Key::Escape)) app.Close();
        controller.SetInputEnabled(!app.GetDebugUI().WantsMouse());
        auto exposure = cam.Exposure();
        
        // --- Panel de edición de escena en runtime ---
        ImGui::Begin("Escena");
        // Atajos globales: solo empujan al cambiar (no cada frame), asi el toggle
        // individual de cada behavior en el inspector no se pierde.
        if (ImGui::Button("Guardar escena")) {
            serialization::SceneSerializer::Save("assets/scenes/sandbox.efe",
                                                 scene, assets, rm, registry);
        }
        ImGui::Separator();
        if (ImGui::Checkbox("Animate", &animate)) {
            setEnabled(ratHandle,  animate);
            setEnabled(orbitLight, animate);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Animar Sol", &animateSun)) setEnabled(sunNode, animateSun);
        ImGui::SliderFloat("Ambient", &scene.ambientFactor, 0.0f, 1.0f);
        if(ImGui::SliderFloat("Exposure", &exposure, 0.0f, 5.0f) == true) { cam.SetExposure(exposure); }
        renderer::BloomSettings& s = app.GetBloomPass().settings(); 
        if (ImGui::CollapsingHeader("Bloom")) {
            ImGui::SliderFloat("Threshold",  &s.threshold,  0.0f, 5.0f);
            ImGui::SliderFloat("Knee",       &s.knee,       0.0f, 1.0f);
            ImGui::SliderFloat("Intensity",  &s.intensity,  0.0f, 0.5f);
            ImGui::SliderInt  ("Iterations", &s.iterations, 1,    10);
}
        renderer::FxaaSettings& fx = app.GetFxaaPass().settings();
        if (ImGui::CollapsingHeader("FXAA")) {
            ImGui::Checkbox("Enabled", &fx.enabled);
        }

        if (ImGui::CollapsingHeader("Sombras", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderer::ShadowSettings& sh = app.GetShadowPass().settings();
            ImGui::Checkbox   ("Habilitadas",    &sh.enabled);
            ImGui::SliderFloat("Ortho HalfSize", &sh.orthoHalfSize, 5.0f,  200.0f);
            ImGui::SliderFloat("Distancia Luz",  &sh.distance,      10.0f, 400.0f);
            ImGui::SliderFloat("Near",           &sh.nearPlane,     0.1f,  50.0f);
            ImGui::SliderFloat("Far",            &sh.farPlane,      50.0f, 600.0f);
            ImGui::SliderFloat("Bias Min",       &sh.biasMin,       0.0f,  0.01f,  "%.4f");
            ImGui::SliderFloat("Bias Max",       &sh.biasMax,       0.0f,  0.02f,  "%.4f");
        }

        ImGui::End();

        // grafo con nodos de arbol
        ImGui::Begin("Jerarquia");

        std::function<void(scene::NodeHandle)> drawNode = [&](scene::NodeHandle h) {
            scene::Node& node = scene.Get(h);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                                     | ImGuiTreeNodeFlags_SpanAvailWidth
                                     | ImGuiTreeNodeFlags_DefaultOpen;
            if (node.children.empty())
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (h == selected) flags |= ImGuiTreeNodeFlags_Selected;

            // Marca de que adjunto tiene el nodo: malla, luz, o si es solo transform
            const char* tag = node.mesh ? "[M]" : (node.light ? "[L]" : "[T]");
            // Los behaviors no son exclusivos con malla/luz, van como sufijo aparte.
            const char* behTag = node.behaviors.empty() ? "" : " [B]";

            ImGui::PushID((int)h.index);
            const bool open = ImGui::TreeNodeEx("nodo", flags, "%s %s%s", tag, node.name.c_str(), behTag);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) selected = h;

            if (open && !node.children.empty()) {
                const std::vector<scene::NodeHandle> kids2 = node.children;
                for (scene::NodeHandle c : kids2) {
                    if (scene.IsValid(c)) drawNode(c);
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        };
        drawNode(scene.Root());

        ImGui::Separator();

        if (scene.IsValid(selected)) {
            scene::Node& node = scene.Get(selected);
            ImGui::Text("Nodo: %s", node.name.c_str());
            ImGui::TextDisabled("handle { index=%u, gen=%u }", node.self.index, node.self.generation);

            // El world es derivado: se muestra como lectura, no se edita.
            const glm::vec3 worldPos = glm::vec3(node.worldMatrix[3]);
            ImGui::TextDisabled("mundo: %.2f, %.2f, %.2f", worldPos.x, worldPos.y, worldPos.z);

            math::Transform t = node.local;
            bool changed = false;
            changed |= ImGui::DragFloat3("Posicion local", glm::value_ptr(t.position), 0.1f);
            changed |= ImGui::DragFloat3("Rotacion local", glm::value_ptr(t.rotation), 0.5f);
            changed |= ImGui::DragFloat3("Escala local",   glm::value_ptr(t.scale),    0.05f);
            if (changed) scene.SetLocalTransform(selected, t);

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
                const char* kindName = node.light->kind == scene::LightKind::Point ? "Point" : "Directional";
                ImGui::TextDisabled("luz: %s", kindName);
                const f32 maxInt = node.light->kind == scene::LightKind::Point ? 20000.0f : 20.0f;
                const f32 speed  = node.light->kind == scene::LightKind::Point ? 10.0f : 0.05f;
                ImGui::DragFloat3("Color/Int", glm::value_ptr(node.light->color), speed, 0.0f, maxInt);
            }

            if (ImGui::Button("Crear hijo")) {
                char name[32];
                std::snprintf(name, sizeof(name), "nodo_%u", spawnCounter++);
                scene.CreateChild(selected, name);
            }
            ImGui::SameLine();
            if (selected != scene.Root()) {
                if (ImGui::Button("Destruir")) {
                    scene.Destroy(selected);
                    selected = scene::NodeHandle{};
                }
            }
        } else {
            ImGui::TextDisabled("Ningun nodo seleccionado");
        }

        ImGui::End();
        
        scene.Update(app.DeltaTime());

        app.RenderScene(scene, cam);
        app.EndFrame();
    }

    EF_LOG_INFO("=== efengine: sandbox shutdown ===");
    return 0;
}
