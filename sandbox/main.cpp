#include <efengine/application/Application.h>
#include <efengine/scene/CameraController.h>
#include <efengine/renderer/VertexLayout.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Buffer.h>
#include <efengine/scene/Camera.h>
#include <efengine/core/Types.h>
#include <efengine/core/Log.h>
#include <efengine/resources/FileIO.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <utility>

/* https://learnopengl.com/Lighting/Basic-Lighting */ 

namespace {
    using namespace efengine;

    u32 planeIndexes[] = { 0,1,2, 2,3,0};
    f32 vertices[] = {
        // posición            // normal           // uv        // tangente
        -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,   1.0f, 0.0f, 0.0f, // 0 inf-izq
        0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f,   1.0f, 0.0f, 0.0f, // 1 inf-der
        0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f,   1.0f, 0.0f, 0.0f, // 2 sup-der
        -0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f,   1.0f, 0.0f, 0.0f, // 3 sup-izq
    };

    std::optional<std::string> LoadFromFS(const char* path) {
        auto result = resources::FileIO::ReadText(path);
        return result;
    }

    scene::Camera setupCamera(f32 aspect) {
        scene::Camera cam;
        cam.SetAspect(aspect);

        return cam;
    }

    renderer::VertexLayout setupVertexLayout() {
        renderer::VertexLayout layout;

        layout.Push(renderer::ShaderDataType::Float3);  // posición  loc 0
        layout.Push(renderer::ShaderDataType::Float3);  // normal    loc 1
        layout.Push(renderer::ShaderDataType::Float2);  // uv        loc 2
        layout.Push(renderer::ShaderDataType::Float3);  // tangente  loc 3

        return layout;
    }

    renderer::VertexArray setupVertexArray(renderer::Buffer vbo, renderer::IndexBuffer ebo, const renderer::VertexLayout& layout) {
        renderer::VertexArray va;

        va.AddVertexBuffer(std::move(vbo), layout);
        va.SetIndexBuffer(std::move(ebo));

        return va;
    }
}

int main() {

    EF_LOG_INFO("=== efengine: sandbox ===");

    application::Application app;
    platform::Window&   window = app.GetWindow();
    renderer::Renderer& gfx    = app.GetRenderer();

    // SETUP - Inicialización de Shaders
    const char* vshaderPath = "assets/shaders/pbr.vert";
    const char* fshaderPath = "assets/shaders/pbr.frag";
    std::optional<std::string> vertexShaderStream = LoadFromFS(vshaderPath);
    std::optional<std::string> fragmentShaderStream = LoadFromFS(fshaderPath);

    if(!vertexShaderStream || !fragmentShaderStream) 
    { 
        EF_LOG_ERROR("Error cargando shaders");
        return 1; 
    }

    auto shaderOpt = renderer::Shader::Create(vertexShaderStream->c_str(), fragmentShaderStream->c_str());
    if (!shaderOpt) 
    { 
        EF_LOG_ERROR("Error creando shaders");
        return 1; 
    } 
    
    // SETUP - Inicialización de Textura y Material
    const char* rock_albedo = "assets/textures/rock/rock_diffuse.png";
    const char* rock_roughness = "assets/textures/rock/rock_roughness.png";
    const char* rock_displacement = "assets/textures/rock/rock_displacement.png";
    const char* rock_normal = "assets/textures/rock/rock_normal.png";
    const char* rock_ao = "assets/textures/rock/rock_ao.png";



    // TODO FFONTANA - HELPER PARA CARGAR MAPAS PBR, Y ALBEDO APARTE
    if (!rock_albedo || !rock_roughness || !rock_displacement || !rock_normal || !rock_ao) 
    {
        EF_LOG_ERROR("No se pudo cargar alguno de los mapas de textura");
        return 1;
    }

    auto albedoOpt = renderer::Texture::Create(rock_albedo, renderer::ColorSpace::sRGB);
    auto roughnessOpt = renderer::Texture::Create(rock_roughness); // default - linear
    auto normalOpt = renderer::Texture::Create(rock_normal);
    auto aoOpt = renderer::Texture::Create(rock_normal);




    renderer::Material material (&*shaderOpt);
    material.SetAlbedoMap(&*albedoOpt);
    material.SetRoughnessMap(&*roughnessOpt);
    material.SetNormalMap(&*normalOpt);
    material.SetAOMap(&*aoOpt);
    

    // SETUP - Vertex Objects
    renderer::Buffer       vbo(vertices, sizeof(vertices));
    renderer::IndexBuffer  ebo(planeIndexes, 6);
    renderer::VertexLayout layout = setupVertexLayout();
    renderer::VertexArray va = setupVertexArray(std::move(vbo), std::move(ebo), layout);

    // SETUP -  Inicialización de Escena
    glm::vec3 lightPos  = glm::vec3(1.2f, 1.0f, 2.0f);
    glm::vec3 lightColor = glm::vec3(25.0f);
    scene::Camera cam = setupCamera(window.GetAspectRatio());
    scene::CameraController controller(&cam);

    window.SetEventListener(&controller);
    f64 lastTime = window.GetTime();
    f32 angle = 0.0f;



    // LOOP
    while (!window.ShouldClose()) {
        window.PollEvents();
        if (window.IsKeyPressed(platform::Key::Escape)) {
            window.SetShouldClose(true);
        }

        // calculo el delta al inicio del loop
        f64 now = platform::Window::GetTime();
        f32 deltaTime = (f32)(now - lastTime);
        lastTime = now;

        gfx.Clear(0.1f, 0.1f, 0.12f, 1.0f);
        
        shaderOpt->Bind(); // activa el shader

        angle += deltaTime * glm::radians(50.0f); // 50 grados por segundo no importa los fps
        glm::mat4 model = glm::mat4(1.0f); 

        shaderOpt->SetMat4("uModel", model);
        shaderOpt->SetMat4("uView", cam.ViewMatrix());
        shaderOpt->SetMat4("uProjection", cam.ProjectionMatrix());
        shaderOpt->SetVec3("uViewPos", cam.Position());
        shaderOpt->SetInt("uLightCount", 1);
        shaderOpt->SetVec3("uLightPositions[0]", lightPos);
        shaderOpt->SetVec3("uLightColors[0]", lightColor);
        shaderOpt->SetFloat("uAmbientFactor", 0.3f);

        // bind del shader
        material.Bind();

        // Luego dibujar
        gfx.Draw(va, *shaderOpt);
        window.SwapBuffers();
    }

    EF_LOG_INFO("=== efengine: sandbox shutdown ===");
    return 0;
}
