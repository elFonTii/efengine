#pragma once

#include <efengine/core/Types.h>
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

            // Par Begin/End que cubre el viewport con un host invisible y le mete un
            // DockSpace. Begin devuelve el id del dockspace y End lo submitea.
            //
            // Van en par y en ese orden, antes de cualquier ventana que se quiera
            // dockear y despues de la barra de menu (el host se ancla al area de
            // trabajo, que es lo que queda del viewport tras descontarla).
            //
            // Estan separados a proposito: DockSpace() crea el nodo si no existe, asi
            // que un layout por defecto solo puede detectar "todavia no hay layout" y
            // construirse en el medio de los dos. En el medio va DockBuilder y nada
            // mas: cualquier widget que se submitee ahi cae dentro del host invisible.
            //
            // El nodo central queda passthrough: la escena 3D del backbuffer se ve por
            // el agujero, no hace falta renderizarla a textura.
            u32  BeginDockspace();
            void EndDockspace(u32 dockspaceId);

            bool WantsMouse() const;
            bool WantsKeyboard() const;
    };

}
}
