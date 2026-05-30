#pragma once

namespace efengine {
namespace platform { class Window; } // forward declaration

namespace renderer {

    // Encapsula la inicialización de OpenGL para una ventana:
    // carga los punteros de función con GLAD y reporta info de la GPU.
    class Context {
        public:
            // Requiere que el contexto GL de 'window' ya esté current
            // (lo garantiza el ctor de Window).
            explicit Context(platform::Window& window);

            // No posee recursos liberables (GLAD es global): defaults bastan.
            Context(const Context&)            = delete;
            Context& operator=(const Context&) = delete;

        private:
            void log_gpu_info() const;
    };

} // namespace renderer
} // namespace efengine
