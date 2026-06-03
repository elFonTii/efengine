#include <efengine/application/Application.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Buffer.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/renderer/VertexLayout.h>
#include <efengine/core/Log.h>
#include <efengine/core/Types.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>   // GLFW_KEY_ESCAPE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <utility>

/* https://learnopengl.com/Lighting/Basic-Lighting */ 

namespace {
    const char* kVertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 vFragPos;
out vec3 vNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    vFragPos = vec3(uModel * vec4(aPos, 1.0));
    vNormal = mat3(transpose(inverse(uModel))) * aNormal; // normal matrix, corrige los normales en el escalado no uniforme
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0); // Model -> View -> Projection 
}
)";

    const char* kFragmentSrc = R"(#version 330 core
in vec3 vFragPos;
in vec3 vNormal;
out vec4 FragColor;

uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

void main() {
    vec3  objectColor     = vec3(1.0, 0.5, 0.31);  // coral;
    float ambientStrength = 0.1;
    float specularStrength = 0.5;
    float shininess       = 32.0;

    vec3 ambient = ambientStrength * uLightColor;
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vFragPos);
    vec3 diffuse = max(dot(N, L), 0.0) * uLightColor;
    vec3 V = normalize(uViewPos - vFragPos);
    vec3 R = reflect(-L, N);

    float spec = pow(max(dot(V, R), 0.0), shininess);
    vec3 specular = specularStrength * spec * uLightColor;
    FragColor = vec4((ambient + diffuse + specular) * objectColor, 1.0);
}
)";

    f32 vertices[] = {
    // posición            // normal
    // Cara atrás (z = -0.5), normal (0, 0, -1)
    -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,

    // Cara frente (z = 0.5), normal (0, 0, 1)
    -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,

    // Cara izquierda (x = -0.5), normal (-1, 0, 0)
    -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,

    // Cara derecha (x = 0.5), normal (1, 0, 0)
     0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,

    // Cara abajo (y = -0.5), normal (0, -1, 0)
    -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,

    // Cara arriba (y = 0.5), normal (0, 1, 0)
    -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,
    };
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
    
    renderer::Buffer       vbo(vertices, sizeof(vertices));
    renderer::VertexLayout layout;
    layout.Push(renderer::ShaderDataType::Float3);   // posición
    layout.Push(renderer::ShaderDataType::Float3);   // uv -> normal


    renderer::VertexArray va;
    va.AddVertexBuffer(std::move(vbo), layout);

    while (!window.ShouldClose()) {
        window.PollEvents();
        if (window.IsKeyPressed(GLFW_KEY_ESCAPE)) {
            window.SetShouldClose(true);
        }

        gfx.Clear(0.1f, 0.1f, 0.12f, 1.0f);

        // Luego dibujar el cubo
        shaderOpt->Bind();
        gfx.Draw(va, *shaderOpt);
        window.SwapBuffers();
    }

    EF_LOG_INFO("=== Sandbox finalizado limpiamente ===");
    return 0;
}
