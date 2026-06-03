#include <efengine/application/Application.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Buffer.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/renderer/VertexLayout.h>
#include <efengine/renderer/IndexBuffer.h>
#include <efengine/renderer/Texture.h>
#include <efengine/core/Log.h>
#include <efengine/core/Types.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>   // GLFW_KEY_ESCAPE

#include <utility>

namespace {
    const char* kVertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
out vec2 vUV;
void main() {
    vUV = aUV;
    gl_Position = vec4(aPos, 1.0);
}
)";

    const char* kFragmentSrc = R"(#version 330 core
out vec4 FragColor;
in vec2 vUV;
uniform sampler2D uAlbedo;
void main() {
    FragColor = texture(uAlbedo, vUV);
}
)";
}

int main() {
    using namespace efengine;

    EF_LOG_INFO("=== efengine Sandbox: Fase 4 (Albedo) ===");

    application::Application app;
    platform::Window&   window = app.GetWindow();
    renderer::Renderer& gfx    = app.GetRenderer();

    auto shaderOpt = renderer::Shader::Create(kVertexSrc, kFragmentSrc);
    if (!shaderOpt) {
        EF_LOG_ERROR("No se pudo crear el shader; abortando");
        return 1;
    }

    auto texOpt = renderer::Texture::Create("assets/madera.jpg");

    if (!texOpt) {
        EF_LOG_ERROR("No se pudo cargar la textura; abortando");
        return 1;
    }

    // QUAD: 4 vertices vec3 + uv, 6 indices (2 triángulos)
    f32 vertices[] = {
        // posición            // uv
        -0.5f, -0.5f, 0.0f,    0.0f, 0.0f,   // 0: inferior-izquierda
         0.5f, -0.5f, 0.0f,    1.0f, 0.0f,   // 1: inferior-derecha
         0.5f,  0.5f, 0.0f,    1.0f, 1.0f,   // 2: superior-derecha
        -0.5f,  0.5f, 0.0f,    0.0f, 1.0f,   // 3: superior-izquierda
    };

    u32 indices[] = { 0, 1, 2, 2, 3, 0 };
    
    renderer::Buffer       vbo(vertices, sizeof(vertices));
    renderer::VertexLayout layout;
    layout.Push(renderer::ShaderDataType::Float3);   // posición
    layout.Push(renderer::ShaderDataType::Float2);   // uv


    renderer::VertexArray va;
    va.AddVertexBuffer(std::move(vbo), layout);
    va.SetIndexBuffer(renderer::IndexBuffer(indices, 6)); // Se mueve el EBO al VAO, el VAO toma propiedad del recurso

    while (!window.ShouldClose()) {
        window.PollEvents();
        if (window.IsKeyPressed(GLFW_KEY_ESCAPE)) {
            window.SetShouldClose(true);
        }

        gfx.Clear(0.1f, 0.1f, 0.12f, 1.0f);
        shaderOpt->Bind();
        shaderOpt->SetInt("uAlbedo", 0);
        texOpt->Bind(0);
        gfx.Draw(va, *shaderOpt);
        window.SwapBuffers();
    }

    EF_LOG_INFO("=== Sandbox finalizado limpiamente ===");
    return 0;
}
