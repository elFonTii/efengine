#include <efengine/application/Application.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/Vertex.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/DirectionalLight.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/CameraController.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/Node.h>
#include <efengine/math/Transform.h>
#include <efengine/core/Types.h>
#include <efengine/core/Log.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>

#include <imgui.h>
#include <cstdio>

#include <optional>
#include <string>
#include <cmath>
#include <unordered_map>
#include <functional>
#include <vector>

namespace {
    using namespace efengine;

    std::optional<renderer::Material> makePbrMaterial(
            resources::ResourceManager& rm, const renderer::Shader* shader,
            const std::string& base, const std::string& res, const std::string& ext,
            bool withHeight) {
        auto tex = [&](const char* map, renderer::ColorSpace space) {
            return rm.GetTexture((base + map + "_" + res + ext).c_str(), space);
        };

        renderer::Texture* albedo = tex("diff",   renderer::ColorSpace::sRGB);
        renderer::Texture* normal = tex("nor_gl", renderer::ColorSpace::Linear);
        renderer::Texture* rough  = tex("rough",  renderer::ColorSpace::Linear);
        renderer::Texture* ao     = tex("ao",     renderer::ColorSpace::Linear);
        renderer::Texture* height = withHeight ? tex("disp", renderer::ColorSpace::Linear) : null;

        if (!albedo || !normal || !rough || !ao || (withHeight && !height)) {
            return std::nullopt;
        }

        renderer::Material mat(shader);
        mat.SetAlbedoMap(albedo);
        mat.SetNormalMap(normal);
        mat.SetRoughnessMap(rough);
        mat.SetAOMap(ao);
        if (height) mat.SetHeightMap(height);
        return mat;
    }

    renderer::Model makePlane(const std::string& materialName, f32 halfSize, f32 tiles) {
        const std::vector<renderer::Vertex> vertices = {
            // position                            normal         uv                tangent
            { {-halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f,  0.0f},  {1.0f, 0.0f, 0.0f} },
            { { halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {tiles, 0.0f},  {1.0f, 0.0f, 0.0f} },
            { { halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {tiles, tiles}, {1.0f, 0.0f, 0.0f} },
            { {-halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f,  tiles}, {1.0f, 0.0f, 0.0f} },
        };
        const std::vector<u32> indices = { 0, 3, 2, 2, 1, 0 }; // CCW visto desde +Y: winding concuerda con la normal (0,1,0)

        std::vector<renderer::Mesh> meshes;
        meshes.emplace_back(vertices, indices, materialName);
        return renderer::Model(std::move(meshes));
    }
}

int main() {
    using namespace efengine;

    EF_LOG_INFO("=== efengine: sandbox street rat ===");

    application::Application app;
    app.SetClearColor(0.18f, 0.18f, 0.18f);
    resources::ResourceManager& rm = app.GetResources();

    renderer::Shader* pbr  = rm.GetShader("pbr", "assets/shaders/pbr.vert", "assets/shaders/pbr.frag");
    renderer::Model*  rat  = rm.GetModel("assets/models/street_rat_4k.fbx");
    renderer::Model*  lamp = rm.GetModel("assets/models/industrial_pipe_lamp_2k.fbx");

    auto streetRatMatOpt = makePbrMaterial(rm, pbr, "assets/textures/street_rat/street_rat_", "4k", ".jpg", false);
    auto groundMatOpt    = makePbrMaterial(rm, pbr, "assets/textures/brown_mud/brown_mud_03_", "2k", ".jpg", true);
    auto lampMatOpt      = makePbrMaterial(rm, pbr, "assets/textures/industrial_lamp/industrial_pipe_lamp_", "2k", ".jpg", true);

    if (!pbr || !rat || !lamp || !streetRatMatOpt || !groundMatOpt || !lampMatOpt) {
        EF_LOG_ERROR("No se pudieron cargar los recursos");
        return 1;
    }

    renderer::Material streetRatMat = std::move(*streetRatMatOpt);
    renderer::Material groundMat    = std::move(*groundMatOpt);
    renderer::Material lampMat      = std::move(*lampMatOpt);
    groundMat.heightScale = 0.0f;

    lampMat.heightScale   = 0.0f;
    lampMat.metallic   = 0.8f;
    lampMat.roughness = 1.0f;

    renderer::MaterialMap ratMats = {
        { "street_rat",      &streetRatMat },
        { "street_rat_hair", &streetRatMat },
    };

    renderer::MaterialMap lampMats;
    for (const renderer::Mesh& mesh : lamp->meshes()) {
        lampMats[mesh.materialName()] = &lampMat;
    }

    renderer::Model groundModel = makePlane("ground", 300.0f, 24.0f);
    renderer::MaterialMap groundMats = {
        { "ground", &groundMat },
    };

    scene::SceneGraph scene;
    scene.ambientFactor = 0.08f;

    math::Transform ratTransform;
    ratTransform.scale = glm::vec3(10.0f);
    const scene::NodeHandle ratHandle = scene.CreateNode("model_rata");
    scene.AttachMesh(ratHandle, { rat, ratMats });
    scene.SetLocalTransform(ratHandle, ratTransform);

    const scene::NodeHandle groundNode = scene.CreateNode("plano");
    scene.AttachMesh(groundNode, { &groundModel, groundMats });

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

    scene::Camera cam;
    cam.SetAspect(app.GetWindow().GetAspectRatio());
    scene::CameraController controller(&cam);
    app.GetWindow().SetEventListener(&controller);

    bool animate = true;
    bool animateSun = false;

    scene::NodeHandle selected;
    u32 spawnCounter = 0;


    while (app.Running()) {
        app.BeginFrame();
        if (app.IsKeyPressed(platform::Key::Escape)) app.Close();
        controller.SetInputEnabled(!app.GetDebugUI().WantsMouse());
        auto exposure = cam.Exposure();
        
        // --- Panel de edición de escena en runtime ---
        ImGui::Begin("Escena");
        ImGui::Checkbox("Animate", &animate);
        ImGui::SameLine();
        ImGui::Checkbox("Animar Sol", &animateSun);
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

            ImGui::PushID((int)h.index);
            const bool open = ImGui::TreeNodeEx("nodo", flags, "%s %s", tag, node.name.c_str());
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
        
        if (animate) {
            const f32 elapsed = static_cast<f32>(app.Elapsed());
            if (scene.IsValid(orbitLight)) {
                math::Transform lt = scene.Get(orbitLight).local;
                lt.position = glm::vec3(50.0f * std::cos(elapsed), 80.0f, 50.0f * std::sin(elapsed));
                scene.SetLocalTransform(orbitLight, lt);
            }
            if (scene.IsValid(ratHandle)) {
                math::Transform rt = scene.Get(ratHandle).local;
                rt.rotation.y += app.DeltaTime() * 20.0f; // 20 grados/seg
                scene.SetLocalTransform(ratHandle, rt);
            }
        }

        if (animateSun && scene.IsValid(sunNode)) {
            const f32 t = static_cast<f32>(app.Elapsed());
            math::Transform st = scene.Get(sunNode).local;
            st.rotation.y = t * 30.0f;   // gira el sol en Y
            scene.SetLocalTransform(sunNode, st);
        }

        app.RenderScene(scene, cam);
        app.EndFrame();
    }

    EF_LOG_INFO("=== efengine: sandbox shutdown ===");
    return 0;
}
