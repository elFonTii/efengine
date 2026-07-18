#include "Application.h"

#include <efengine/core/Log.h>
#include <efengine/scene/Scene.h>
#include <efengine/scene/SceneObject.h>
#include <efengine/scene/Camera.h>

namespace efengine {
namespace application {

    Application::Application()
        : m_window( platform::WindowProps{ "efengine", 800, 600, true } )
        , m_context( m_window ) {
        // m_renderer se construye por defecto (Rule of Zero, sin estado GL).
        EF_LOG_INFO("Application inicializada");
    }

    void Application::BeginFrame() {
        m_time.Tick();
        m_window.PollEvents();
        m_renderer.Clear(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
    }

    void Application::EndFrame() {
        m_window.SwapBuffers();
    }

    // la tarea es "traducir" la scena a render
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

} // namespace application
} // namespace efengine
