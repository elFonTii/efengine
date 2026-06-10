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

        // Framebuffer
        i32 fbWidth = 0, fbHeight = 0 ;
        glfwGetFramebufferSize(m_handle, &fbWidth, &fbHeight);
        m_width = fbWidth;
        m_height = fbHeight;
        
        glfwSetWindowUserPointer(m_handle, this);
        glfwSetFramebufferSizeCallback(m_handle, OnFramebufferResize);

        glfwSetCursorPosCallback(m_handle, OnCursorPos);
        glfwSetMouseButtonCallback(m_handle, OnMouseButton);
        glfwSetScrollCallback(m_handle, OnScroll);

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
        : m_handle(std::exchange(other.m_handle, null))
        , m_width(std::exchange(other.m_width, 0))
        , m_height(std::exchange(other.m_height, 0))
        , m_listener(std::exchange(other.m_listener, null)){
            if(m_handle != null) {
                glfwSetWindowUserPointer(m_handle, this); // sino apuntaría al objeto original y no al copiado.
            }
        }

    Window& Window::operator=(Window&& other) noexcept {
        if(this != &other) {
            // Liberar nuestro recurso actual si lo tenemos.
            if(m_handle != null) {
                glfwDestroyWindow(m_handle);

                if(--s_window_count == 0) {
                    glfwTerminate();
                }
            }

            // Transferir el recurso de 'other' (siempre, tuviéramos handle o no).
            m_handle = std::exchange(other.m_handle, null);
            m_width  = std::exchange(other.m_width, 0);
            m_height = std::exchange(other.m_height, 0);
            m_listener = std::exchange(other.m_listener, null);

            if(m_handle != null) {
                glfwSetWindowUserPointer(m_handle, this); // sino apuntaría al objeto original y no al copiado.
            }
        }

        return *this;
    }

    void Window::PollEvents() {
        glfwPollEvents();
    }

    void Window::SwapBuffers() {
        glfwSwapBuffers(m_handle);
    }

    bool Window::ShouldClose() const {
        return glfwWindowShouldClose(m_handle);
    }

    void Window::SetShouldClose(bool shouldClose) {
        glfwSetWindowShouldClose(m_handle, shouldClose);
    }

    bool Window::IsKeyPressed(Key _key) const {
        return glfwGetKey(m_handle, (int)_key) == GLFW_PRESS;
    }

    f32 Window::GetAspectRatio() const {
        if(m_width != 0 && m_height != 0) {
            return (f32)m_width / (f32)m_height;
        }

        return 1.0f; // ventana minimizada, se retorna 1 para evitar dividir por 0
    }

    f64 Window::GetTime() {
        return glfwGetTime();
    };

    void Window::SetEventListener(IEventListener* listener) { m_listener = listener; }

    /* Callback que actualiza el tamaño del viewport */
    void Window::OnFramebufferResize(GLFWwindow* handle, int width, int height) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle)); // obtiene el puntero que se guardó con SetWindowUserPointer en el constructor
        EF_ASSERT(self != null, "Window::OnFramebufferResize: glfw user pointer nulo");

        self -> m_width = (u32)width;
        self -> m_height= (u32)height;
        glViewport(0,0, (GLsizei)width, (GLsizei)height); 

        if (self->m_listener) self->m_listener->OnWindowResize(self->m_width, self->m_height);
    }

    /* Callback que captura la posicion del cursor */
    void Window::OnCursorPos(GLFWwindow* handle, double xpos, double ypos) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle));
        EF_ASSERT(self != null, "Window::OnCursorPos: glfw user pointer nulo");

        if (self->m_listener) {
            self->m_listener->OnMouseMove((f32)xpos, (f32)ypos);
        }
    }

    /* Callback que captura los botones del cursor */
    void Window::OnMouseButton(GLFWwindow* handle, int button, int action, int mods) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle));
        EF_ASSERT(self != null, "Window::OnMouseButton: glfw user pointer nulo");
        
        if (self->m_listener) {
            self->m_listener->OnMouseButton((i32)button, (i32)action, (i32)mods);
        }
    }

    /* Callback que captura el scroll del cursor */
    void Window::OnScroll(GLFWwindow* handle, double xoffset, double yoffset) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle));
        EF_ASSERT(self != null, "Window::OnScroll: glfw user pointer nulo");
        
        if (self->m_listener) {
            self->m_listener->OnMouseScroll((f32)xoffset, (f32)yoffset);
        }
    }

    
    
}
}