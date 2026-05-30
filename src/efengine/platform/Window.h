#pragma once

#include <efengine/core/Types.h>
// Forward declaration de GLFW
struct GLFWwindow;

namespace efengine {
namespace platform {

    struct WindowProps {
        const char* title = "EFEngine";
        u32 width = 800;
        u32 height = 600;
        bool vsync = true;
    };

    // === CLASE VENTANA ===
    class Window {
        public:
            explicit Window(const WindowProps& props = WindowProps());
            ~Window();

            // El copy es prohibido
            Window(const Window&) =             delete;
            Window& operator=(const Window&) =  delete;

            // El move es permitido
            Window(Window&&) =             default;
            Window& operator=(Window&&) =  default;

            // Loop
            void PollEvents();
            void SwapBuffers();
            bool ShouldClose() const;
            void SetShouldClose(bool shouldClose);

            // Input
            bool IsKeyPressed(int key) const;

            // Accessors
            u32 GetWidth() const { return m_width; }
            u32 GetHeight() const { return m_height; }
            GLFWwindow* GetNativeHandle() const { return m_handle; }

        private:
        GLFWwindow* m_handle = null;
        u32 m_width = 0;
        u32 m_height = 0;
        
    };
}
}