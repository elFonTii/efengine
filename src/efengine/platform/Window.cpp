#include "Window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <utility>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace { u32 s_window_count = 0;}

namespace efengine {
namespace platform {

    Window::Window(const WindowProps& props) { 
        m_width = props.width;
        m_height = props.height;

        if(s_window_count == 0) {
            const int glfwInitResult = glfwInit();
            EF_ASSERT(glfwInitResult, "Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        

        m_handle = glfwCreateWindow(m_width, m_height, props.title, null, null);
        EF_ASSERT(m_handle != null, "Failed to create GLFW window");
        ++s_window_count;

        glfwMakeContextCurrent(m_handle);
        glfwSwapInterval(props.vsync ? 1 : 0);

        EF_LOG_INFO("Window created: %s (%d x %d)", props.title, m_width, m_height);
    };

    Window::~Window() { 
        if(m_handle != null) {
            glfwDestroyWindow(m_handle);

            m_handle = null;
            --s_window_count;

            if(s_window_count == 0) {
                glfwTerminate();
            }

            EF_LOG_INFO("Window destroyed");
        }
    };

    Window::Window(Window&& other) noexcept
        :  m_handle(std::exchange(other.m_handle, nullptr)) {}
            /*
            m_handle(other.m_handle) {
            other.m_handle = null; // Si no al hacer el move ambos creerian poseerlo momentaneamente.
            */

    Window& Window::operator=(Window&& other) noexcept {
        if(this != &other) {
            if(m_handle != null) {
                glfwDestroyWindow(m_handle);
                
                if(--s_window_count < 1) {
                    glfwTerminate();
                    EF_ASSERT(s_window_count == 0, "Window count should be zero after destroying the last window.");
                }

                m_handle = other.m_handle;
                other.m_handle = null; // vaciamos origen, igual al move.
            }
            return *this;
        }
    }
}
}