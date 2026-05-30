#pragma once

#include <efengine/platform/Window.h>
#include <efengine/renderer/Context.h>

namespace efengine {
namespace application {

    class Application {
        public:
            Application();
            void Run();

        private:
            // ORDEN CRÍTICO: m_window se construye PRIMERO (crea + activa el
            // contexto GL); m_context DESPUÉS (carga GLAD sobre ese contexto).
            // El orden de inicialización en C++ sigue el orden de declaración.
            platform::Window  m_window;
            renderer::Context m_context;
    };

} // namespace application
} // namespace efengine
