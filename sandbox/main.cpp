#include <efengine/application/Application.h>
#include <efengine/scene/CameraController.h>
#include <efengine/renderer/VertexLayout.h>
#include <efengine/renderer/VertexArray.h>
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


}

int main() {

    EF_LOG_INFO("=== efengine Sandbox: Fase 5 (Phong) ===");

    application::Application app;
    platform::Window&   window = app.GetWindow();
    renderer::Renderer& gfx    = app.GetRenderer();

    // cargar shaders desde filesystem
    auto vsSrc = resources::FileIO::ReadText("assets/shaders/pbr.vert");
    auto fsSrc = resources::FileIO::ReadText("assets/shaders/pbr.frag");

    if (!vsSrc || !fsSrc) {
        EF_LOG_ERROR("No se pudieron leer los shaders");
        return 1;
    }

    auto shaderOpt = renderer::Shader::Create(vsSrc->c_str(), fsSrc->c_str());
    if (!shaderOpt) {
        EF_LOG_ERROR("No se pudo crear el shader; abortando");
        return 1;
    }
    
    renderer::Buffer       vbo(vertices, sizeof(vertices));
    renderer::IndexBuffer  ebo(planeIndexes, 6);
    
    renderer::VertexLayout layout;
    layout.Push(renderer::ShaderDataType::Float3);  // posición  loc 0
    layout.Push(renderer::ShaderDataType::Float3);  // normal    loc 1
    layout.Push(renderer::ShaderDataType::Float2);  // uv        loc 2
    layout.Push(renderer::ShaderDataType::Float3);  // tangente  loc 3

    renderer::VertexArray va;
    va.AddVertexBuffer(std::move(vbo), layout);
    va.SetIndexBuffer(std::move(ebo));

    glm::vec3 lightPos  = glm::vec3(1.2f, 1.0f, 2.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

    scene::Camera cam;
    cam.SetAspect(window.GetAspectRatio());
    
    scene::CameraController controller(&cam);
    window.SetEventListener(&controller);


    f64 lastTime = window.GetTime();
    f32 angle = 0.0f;



    // TODO: FFONTANA - Optimizacion: agregar continue al inicio para evitar el render con ventana minimizada.
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
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.5f, 1.0f, 0.0f));

        shaderOpt->SetMat4("uModel", model);
        shaderOpt->SetMat4("uView", cam.ViewMatrix());
        shaderOpt->SetMat4("uProjection", cam.ProjectionMatrix());
        shaderOpt->SetVec3("uViewPos", cam.Position());
        shaderOpt->SetVec3("uLightPos", lightPos);
        shaderOpt->SetVec3("uLightColor", lightColor);

        // Luego dibujar
        gfx.Draw(va, *shaderOpt);
        window.SwapBuffers();
    }

    EF_LOG_INFO("=== Sandbox finalizado limpiamente ===");
    return 0;
}
