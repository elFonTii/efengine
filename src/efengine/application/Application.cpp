#include "Application.h"

#include <efengine/core/Log.h>
#include <efengine/scene/Scene.h>
#include <efengine/scene/SceneObject.h>
#include <efengine/scene/Camera.h>

namespace efengine {
namespace application {

    Application::Application()
        : m_window( platform::WindowProps{ "efengine", 800, 600, true } )
        , m_context( m_window )
        , m_debugUI( m_window ) {
        // m_renderer se construye por defecto (Rule of Zero, sin estado GL).
        EF_LOG_INFO("Application inicializada");
    }

    void Application::BeginFrame() {
        m_time.Tick();
        m_window.PollEvents();
        m_renderer.Clear(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
        m_debugUI.NewFrame();   // abre el frame de ImGui; el cliente ya puede emitir widgets
    }

    void Application::EndFrame() {
        m_debugUI.Render();     // dibuja el overlay sobre la imagen final (backbuffer)
        m_window.SwapBuffers();
    }

    void Application::RenderScene(const scene::Scene& scene, const scene::Camera& camera) {
        m_renderer.BeginScene(camera.ViewMatrix(), camera.ProjectionMatrix(), camera.Position(), scene.lights(), scene.ambientFactor);

        // enviar los scene objects
        for(const scene::SceneObject& obj : scene.objects()) {
            if(!obj.model) {
                EF_LOG_WARNING("Se intenta renderizar un objeto sin modelo.");
                continue;
            } else {
                m_renderer.Submit(*obj.model, obj.materials, obj.transform.Matrix());
            }
        }
    }

    f32 Application::DeltaTime() const { return m_time.DeltaTime(); }

    f64 Application::Elapsed() const { return m_time.Elapsed(); }
    
    bool Application::IsKeyPressed(platform::Key key) const { return m_window.IsKeyPressed(key); }

    void Application::Close() { m_window.SetShouldClose(true); }
    
    void Application::SetClearColor(f32 r, f32 g, f32 b, f32 a) {
    m_clearColor[0] = r; m_clearColor[1] = g; m_clearColor[2] = b; m_clearColor[3] = a;
}
    
}
}
