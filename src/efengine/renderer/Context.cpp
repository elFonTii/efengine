#include "Context.h"

#include <glad/gl.h>          // ANTES que GLFW (ver concepto C.2 del plan).

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>       // glfwGetProcAddress

#include <efengine/platform/Window.h>
#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace renderer {

    Context::Context(platform::Window& window) {
        const int version = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
        EF_ASSERT(version != 0, "gladLoadGL failed: could not load OpenGL functions");
        
        glViewport(0, 0, (GLsizei)window.GetWidth(), (GLsizei)window.GetHeight()); // Si lo inicializo en Window el contexto de opengl no está activo todavía "access violation -1073741819"
        glEnable(GL_DEPTH_TEST); // Test de profundidad que descarta fragmentos ocultos (las tapas traseras de una forma no tapen las delanteras)
        log_gpu_info();
    }

    void Context::log_gpu_info() const {
        // glGetString devuelve const GLubyte*; casteamos a const char* para loguear.
        EF_LOG_INFO("OpenGL Version : %s", (const char*)glGetString(GL_VERSION));
        EF_LOG_INFO("GPU Renderer   : %s", (const char*)glGetString(GL_RENDERER));
        EF_LOG_INFO("GPU Vendor     : %s", (const char*)glGetString(GL_VENDOR));
        EF_LOG_INFO("GLSL Version   : %s", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    }

} // namespace renderer
} // namespace efengine
