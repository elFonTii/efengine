#pragma once

#include <efengine/platform/Window.h>

namespace efengine {
namespace application {

    // Handle-owner RAII del contexto de Dear ImGui + sus backends GLFW/OpenGL3.
    // El ctor adquiere (crea el contexto + inicializa los backends sobre la
    // Window ya viva); el dtor libera (shutdown de backends + destroy context).
    // No tiene datos propios: ImGui guarda todo su estado en un contexto GLOBAL
    // (ImGui::GetCurrentContext()); esta clase es sólo un guardián de ciclo de vida.
    //
    // MOVE = delete (a diferencia de Window/Shader): el contexto de ImGui es
    // estado global, no un handle transferible con std::exchange. "Mover" un
    // DebugUI sería una mentira sobre la propiedad. Como es un miembro fijo de
    // Application que nunca se mueve, borrar copy Y move es correcto y honesto.
    class DebugUI {
        public:
            explicit DebugUI(platform::Window& window);  // adquiere: contexto + backends
            ~DebugUI();

            DebugUI(const DebugUI&)            = delete;
            DebugUI& operator=(const DebugUI&) = delete;
            DebugUI(DebugUI&&)                 = delete;  // ImGui = estado global, no movible
            DebugUI& operator=(DebugUI&&)      = delete;

            void NewFrame();             // abre el frame de ImGui (backends + ImGui::NewFrame)
            void Render();               // vuelca los draw-data al backbuffer

            bool WantsMouse() const;     // ImGui::GetIO().WantCaptureMouse
            bool WantsKeyboard() const;  // ImGui::GetIO().WantCaptureKeyboard
    };

} // namespace application
} // namespace efengine
