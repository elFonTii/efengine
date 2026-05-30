#include "Application.h"

#include <glad/gl.h>          // ANTES que GLFW: glClear / glClearColor.

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>       // GLFW_KEY_ESCAPE

#include <efengine/core/Log.h>

namespace efengine {
namespace application {

    Application::Application()
        : m_window( platform::WindowProps{ "efengine", 800, 600, true } )
        , m_context( m_window ) {
        EF_LOG_INFO("Application inicializada");
    }

    void Application::Run() {
        EF_LOG_INFO("Entrando al loop principal");

        while(!m_window.ShouldClose()) {
            m_window.PollEvents();

            if(m_window.IsKeyPressed(GLFW_KEY_ESCAPE)) {
                m_window.SetShouldClose(true);
            }

            glClearColor(0.1f, 0.1f, 0.12f, 1.0f);   // fondo oscuro sólido
            glClear(GL_COLOR_BUFFER_BIT);

            m_window.SwapBuffers();
        }

        EF_LOG_INFO("Saliendo del loop principal");
    }

} // namespace application
} // namespace efengine
