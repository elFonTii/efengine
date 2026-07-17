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
#include <glm/glm.hpp>

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
    platform::Window&          window = app.GetWindow();
    renderer::Renderer&        gfx    = app.GetRenderer();
    resources::ResourceManager& rm    = app.GetResources();
    core::Time& time                  =  app.GetTime();

    renderer::Shader* pbr = rm.GetShader("pbr", "assets/shaders/pbr.vert", "assets/shaders/pbr.frag");
    renderer::Model*  rat = rm.GetModel("assets/models/street_rat_4k.fbx");

    auto streetRatMatOpt = makePbrMaterial(rm, pbr, "assets/textures/street_rat/street_rat_", "4k", ".jpg", false);
    auto groundMatOpt    = makePbrMaterial(rm, pbr, "assets/textures/brown_mud/brown_mud_03_", "2k", ".png", true);

    if (!pbr || !rat || !streetRatMatOpt || !groundMatOpt) {
        EF_LOG_ERROR("No se pudieron cargar los recursos");
        return 1;
    }

    renderer::Material streetRatMat = std::move(*streetRatMatOpt);
    renderer::Material groundMat    = std::move(*groundMatOpt);
    groundMat.heightScale = 0.08f;

    renderer::MaterialMap ratMats = {
        { "street_rat",      &streetRatMat },
        { "street_rat_hair", &streetRatMat },
    };

    renderer::Model groundModel = makePlane("ground", 300.0f, 24.0f);
    renderer::MaterialMap groundMats = {
        { "ground", &groundMat },
    };

    scene::Scene scene;

    math::Transform ratTransform;
    ratTransform.scale = glm::vec3(10.0f);
    const u32 ratHandle = scene.Add({ rat, ratMats, ratTransform });

    scene.Add({ &groundModel, groundMats });

    glm::vec3 lightPos_1 = glm::vec3(-50.0f, 80.0f, 0.0f);
    glm::vec3 lightPos_2 = glm::vec3(0, 90.0f, 0.0f);

    glm::vec3 lightColor = glm::vec3(5000.0f);

    scene::Camera cam;
    cam.SetAspect(window.GetAspectRatio());
    scene::CameraController controller(&cam);
    window.SetEventListener(&controller);

    while (!window.ShouldClose()) {
        time.Tick();
        const f32 elapsed = static_cast<f32>(time.Elapsed());


        window.PollEvents();
        if (window.IsKeyPressed(platform::Key::Escape)) {
            window.SetShouldClose(true);
        }

        gfx.Clear(0.18f, 0.18f, 0.18f, 1.0f);
        
        const glm::vec3 lightPos_0 = glm::vec3( 50.0f * std::cos(elapsed), 80.0f, 50.0f * std::sin(elapsed));

        scene.Get(ratHandle).transform.rotation.y += time.DeltaTime() * 20.0f; // rota la rata 20 grados x seg

        pbr->Bind();
        pbr->SetMat4("uView", cam.ViewMatrix());
        pbr->SetMat4("uProjection", cam.ProjectionMatrix());
        pbr->SetVec3("uViewPos", cam.Position());
        pbr->SetInt("uLightCount", 3);
        pbr->SetVec3("uLightPositions[0]", lightPos_0);
        pbr->SetVec3("uLightColors[0]", lightColor);
        pbr->SetVec3("uLightPositions[1]", lightPos_1);
        pbr->SetVec3("uLightColors[1]", lightColor);
        pbr->SetVec3("uLightPositions[2]", lightPos_2);
        pbr->SetVec3("uLightColors[2]", lightColor);
        pbr->SetFloat("uAmbientFactor", 0.08f);

        for (const scene::SceneObject& obj : scene.objects()) {
            pbr->Bind();
            pbr->SetMat4("uModel", obj.transform.Matrix());
            gfx.Draw(*obj.model, obj.materials);
        }

        window.SwapBuffers();
    }

    EF_LOG_INFO("=== efengine: sandbox shutdown ===");
    return 0;
}
