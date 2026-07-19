#pragma once

#include <efengine/platform/Window.h>

namespace efengine {
namespace application {
    class DebugUI {
        public:
            explicit DebugUI(platform::Window& window);  // adquiere: contexto + backends
            ~DebugUI();

            DebugUI(const DebugUI&)            = delete;
            DebugUI& operator=(const DebugUI&) = delete;
            DebugUI(DebugUI&&)                 = delete; 
            DebugUI& operator=(DebugUI&&)      = delete;

            void NewFrame();             // abre el frame de ImGui (backends + ImGui::NewFrame)
            void Render();

            bool WantsMouse() const;
            bool WantsKeyboard() const;
    };

}
}
