#include "Application.h"

#include <efengine/core/Log.h>

namespace efengine {
namespace application {

    Application::Application()
        : m_window( platform::WindowProps{ "efengine", 800, 600, true } )
        , m_context( m_window ) {
        // m_renderer se construye por defecto (Rule of Zero, sin estado GL).
        EF_LOG_INFO("Application inicializada");
    }

} // namespace application
} // namespace efengine
