#pragma once

#include <efengine/platform/Window.h>
#include <efengine/renderer/Context.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/core/Time.h>

namespace efengine {
namespace application {

    // Bundle RAII de los subsistemas del motor. Expone accessors; el loop
    // principal vive en el cliente (sandbox). m_context no se expone: se
    // retiene solo por su lifetime (RAII de GLAD).
    class Application {
        public:
            Application();

            platform::Window&   GetWindow()   { return m_window; }
            renderer::Renderer& GetRenderer() { return m_renderer; }
            resources::ResourceManager& GetResources() { return m_resources; }
            core::Time& GetTime() { return m_time; }

        private:
            // ORDEN CRÍTICO (contrato, principio 11): Window 1.º (crea + activa
            // el contexto GL); Context 2.º (carga GLAD sobre ese contexto);
            // Renderer 3.º. El orden de init en C++ sigue el orden de declaración.
            // ResourceManager 4.º. (crea recursos cpu, necesita el contexto entero vivo)
            // time no participa del contrato, sólo depende de chrono y el baseline
            // del tiempo se fija en el primer tick, puede ir en cualquier lado.
            platform::Window   m_window;
            renderer::Context  m_context;
            renderer::Renderer m_renderer;
            resources::ResourceManager m_resources;
            core::Time m_time;
    };

} // namespace application
} // namespace efengine
