#include <efengine/application/Application.h>
#include <efengine/resources/ModelLoader.h>
#include <efengine/resources/FileIO.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Shader.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/CameraController.h>
#include <efengine/core/Types.h>
#include <efengine/core/Log.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <unordered_map>

namespace {
    using namespace efengine;

    struct TextureSet {
        std::optional<renderer::Texture> albedo;
        std::optional<renderer::Texture> normal;
        std::optional<renderer::Texture> roughness;
        std::optional<renderer::Texture> ao;
        std::optional<renderer::Texture> height;
        std::optional<renderer::Texture> opacity;
    };

    std::optional<TextureSet> loadStreetRat() {
        const std::string base = "assets/textures/street_rat/street_rat_";

        TextureSet set;
        set.albedo    = renderer::Texture::Create((base + "diff_4k.jpg").c_str(), renderer::ColorSpace::sRGB);
        set.normal    = renderer::Texture::Create((base + "nor_gl_4k.jpg").c_str());
        set.roughness = renderer::Texture::Create((base + "rough_4k.jpg").c_str());
        set.ao        = renderer::Texture::Create((base + "ao_4k.jpg").c_str());
        set.height    = renderer::Texture::Create((base + "height_4k.jpg").c_str());

        if (!set.albedo || !set.normal || !set.roughness || !set.ao || !set.height) {
            return std::nullopt;
        }

        // El street rat no trae mapa de alpha (malla solida, sin recortes).
        return set;
    }

    renderer::Material makeMaterial(const renderer::Shader& shader, const TextureSet& set) {
        renderer::Material mat(&shader);
        mat.SetAlbedoMap(&*set.albedo);
        mat.SetNormalMap(&*set.normal);
        mat.SetRoughnessMap(&*set.roughness);
        mat.SetAOMap(&*set.ao);
       //  mat.SetHeightMap(&*set.height);
        if (set.opacity) {
            mat.SetOpacityMap(&*set.opacity);
        }
        return mat;
    }
}

int main() {
    using namespace efengine;

    EF_LOG_INFO("=== efengine: sandbox street rat ===");

    application::Application app;
    platform::Window&   window = app.GetWindow();
    renderer::Renderer& gfx    = app.GetRenderer();

    auto vsrc = resources::FileIO::ReadText("assets/shaders/pbr.vert");
    auto fsrc = resources::FileIO::ReadText("assets/shaders/pbr.frag");
    if (!vsrc || !fsrc) {
        EF_LOG_ERROR("No se pudieron leer los shaders PBR");
        return 1;
    }

    auto shaderOpt = renderer::Shader::Create(vsrc->c_str(), fsrc->c_str());
    if (!shaderOpt) {
        EF_LOG_ERROR("No se pudo crear el shader PBR");
        return 1;
    }

    auto streetRatTex = loadStreetRat();
    if (!streetRatTex) {
        EF_LOG_ERROR("No se pudieron cargar las texturas del street rat");
        return 1;
    }

    renderer::Material streetRatMat = makeMaterial(*shaderOpt, *streetRatTex);

    // El FBX trae dos materiales (cuerpo y pelo); ambos reusan el set del cuerpo.
    std::unordered_map<std::string, const renderer::Material*> materiales = {
        { "street_rat",      &streetRatMat },
        { "street_rat_hair", &streetRatMat },
    };

    auto modelOpt = resources::ModelLoader::Load("assets/models/street_rat_4k.fbx");
    if (!modelOpt) {
        EF_LOG_ERROR("No se pudo cargar el modelo");
        return 1;
    }

    // El street rat viene en metros (~0.15 u de alto); se escala y se centra
    // sobre el target de la camara (y=200, distancia 500).
    const f32 kEscala = 10.0f;
    glm::mat4 modelMat =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(kEscala));

    glm::vec3 lightPos_0   = glm::vec3(50.0f, 80.0f, 0.0f);
    glm::vec3 lightPos_1   = glm::vec3(-50.0f, 80.0f, 0.0f);
    glm::vec3 lightPos_2  = glm::vec3(0, -80.0f, 0.0f);


    glm::vec3 lightColor = glm::vec3(15000.0f);

    scene::Camera cam;
    cam.SetAspect(window.GetAspectRatio());
    scene::CameraController controller(&cam);
    window.SetEventListener(&controller);

    while (!window.ShouldClose()) {
        window.PollEvents();
        if (window.IsKeyPressed(platform::Key::Escape)) {
            window.SetShouldClose(true);
        }

        gfx.Clear(0.18f, 0.18f, 0.18f, 1.0f);

        shaderOpt->Bind();
        shaderOpt->SetMat4("uModel", modelMat);
        shaderOpt->SetMat4("uView", cam.ViewMatrix());
        shaderOpt->SetMat4("uProjection", cam.ProjectionMatrix());
        shaderOpt->SetVec3("uViewPos", cam.Position());
        shaderOpt->SetInt("uLightCount", 3);
        shaderOpt->SetVec3("uLightPositions[0]", lightPos_0);
        shaderOpt->SetVec3("uLightColors[0]", lightColor);
        shaderOpt->SetVec3("uLightPositions[1]", lightPos_1);
        shaderOpt->SetVec3("uLightColors[1]", lightColor);
        shaderOpt->SetVec3("uLightPositions[2]", lightPos_2);
        shaderOpt->SetVec3("uLightColors[2]", lightColor);
        shaderOpt->SetFloat("uAmbientFactor", 0.08f);

        gfx.Draw(*modelOpt, materiales);

        window.SwapBuffers();
    }

    EF_LOG_INFO("=== efengine: sandbox shutdown ===");
    return 0;
}
