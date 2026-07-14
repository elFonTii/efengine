#pragma once

#include <efengine/platform/Window.h>
#include <efengine/renderer/Context.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/resources/ResourceManager.h>

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

        private:
            // ORDEN CRÍTICO (contrato, principio 11): Window 1.º (crea + activa
            // el contexto GL); Context 2.º (carga GLAD sobre ese contexto);
            // Renderer 3.º. El orden de init en C++ sigue el orden de declaración.
            // ResourceManager 3.º. (crea recursos cpu, necesita el contexto entero vivo)
            platform::Window   m_window;
            renderer::Context  m_context;
            renderer::Renderer m_renderer;
            resources::ResourceManager m_resources;
    };

} // namespace application
} // namespace efengine
