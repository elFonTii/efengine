#include <efengine/application/Application.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/Vertex.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Shader.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/CameraController.h>
#include <efengine/scene/Scene.h>
#include <efengine/scene/SceneObject.h>
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

namespace {
    using namespace efengine;

    // Arma un material PBR pidiendo al manager las texturas por convención de
    // nombre: <base>{diff,nor_gl,rough,ao,disp}_<res><ext>. albedo va en sRGB,
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
        const std::vector<u32> indices = { 0, 1, 2, 2, 3, 0 };

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

    scene::Scene scene;
    scene.ambientFactor = 0.08f;

    math::Transform ratTransform;
    ratTransform.scale = glm::vec3(10.0f);
    const u32 ratHandle = scene.Add({ rat, ratMats, ratTransform });

    scene.Add({ &groundModel, groundMats });

    math::Transform lampTransform;
    lampTransform.position = glm::vec3(30.0f, 0.0f, 0.0f);
    lampTransform.scale    = glm::vec3(1.0f);
    scene.Add({ lamp, lampMats, lampTransform });

    const u32 sun = scene.AddLight({ glm::vec3( 50.0f, 80.0f, 0.0f), glm::vec3(5000.0f) });
    scene.AddLight({ glm::vec3(-50.0f, 80.0f, 0.0f), glm::vec3(5000.0f) });
    scene.AddLight({ glm::vec3(  0.0f, 90.0f, 0.0f), glm::vec3(5000.0f) });

    scene::Camera cam;
    cam.SetAspect(app.GetWindow().GetAspectRatio());
    scene::CameraController controller(&cam);
    app.GetWindow().SetEventListener(&controller);

    bool animate = true;

    while (app.Running()) {
        app.BeginFrame();
        if (app.IsKeyPressed(platform::Key::Escape)) app.Close();

        controller.SetInputEnabled(!app.GetDebugUI().WantsMouse());

        // --- Panel de edición de escena en runtime ---
        ImGui::Begin("Escena");
        ImGui::Checkbox("Animate", &animate);
        ImGui::SliderFloat("Ambient", &scene.ambientFactor, 0.0f, 1.0f);

        if (ImGui::CollapsingHeader("Objetos", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (u32 i = 0; i < scene.objects().size(); ++i) {
                ImGui::PushID((int)i);                 // ids únicos por objeto
                char label[32];
                std::snprintf(label, sizeof(label), "Objeto %u", i);
                if (ImGui::TreeNode(label)) {
                    math::Transform& t = scene.Get(i).transform;
                    ImGui::DragFloat3("Posicion", glm::value_ptr(t.position), 0.1f);
                    ImGui::DragFloat3("Rotacion", glm::value_ptr(t.rotation), 0.5f);   // grados
                    ImGui::DragFloat3("Escala",   glm::value_ptr(t.scale),    0.05f);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        if (ImGui::CollapsingHeader("Luces", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (u32 i = 0; i < scene.lights().size(); ++i) {
                ImGui::PushID(1000 + (int)i);          // rango de ids separado de los objetos
                char label[32];
                std::snprintf(label, sizeof(label), "Luz %u", i);
                if (ImGui::TreeNode(label)) {
                    renderer::PointLight& l = scene.GetLight(i);
                    ImGui::DragFloat3("Posicion",  glm::value_ptr(l.position), 0.5f);
                    ImGui::DragFloat3("Color/Int", glm::value_ptr(l.color),   10.0f, 0.0f, 20000.0f);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        ImGui::End();

        if (animate) {
            const f32 elapsed = static_cast<f32>(app.Elapsed());
            scene.GetLight(sun).position = glm::vec3(50.0f * std::cos(elapsed), 80.0f, 50.0f * std::sin(elapsed));
            scene.Get(ratHandle).transform.rotation.y += app.DeltaTime() * 20.0f; // 20 grados/seg
        }

        app.RenderScene(scene, cam);
        app.EndFrame();
    }

    EF_LOG_INFO("=== efengine: sandbox shutdown ===");
    return 0;
}
