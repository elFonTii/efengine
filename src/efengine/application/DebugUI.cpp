#include "DebugUI.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <efengine/core/Assert.h>

namespace efengine {
namespace application {

    DebugUI::DebugUI(platform::Window& window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        // Sin este flag DockSpace() no hace nada. No se habilita ViewportsEnable:
        // sacar paneles a ventanas del SO trae su propio lote de problemas y no
        // es lo que se pidio.
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        const bool okGlfw = ImGui_ImplGlfw_InitForOpenGL(window.GetNativeHandle(), true);
        EF_ASSERT(okGlfw, "DebugUI: fallo al iniciar el backend GLFW de ImGui");

        // "#version 450" = GLSL de OpenGL 4.5 Core, igual que el contexto de Window.
        const bool okGl = ImGui_ImplOpenGL3_Init("#version 450");
        EF_ASSERT(okGl, "DebugUI: fallo al iniciar el backend OpenGL3 de ImGui");
    }

    DebugUI::~DebugUI() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void DebugUI::NewFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    u32 DebugUI::BeginDockspace() {
        const ImGuiViewport* vp = ImGui::GetMainViewport();

        // WorkPos/WorkSize ya descuentan la barra de menu principal.
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        // El host no se ve: es solo el rectangulo que contiene al dockspace.
        // NoBackground es lo que deja pasar la escena por el nodo central.
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking
                                     | ImGuiWindowFlags_NoTitleBar
                                     | ImGuiWindowFlags_NoCollapse
                                     | ImGuiWindowFlags_NoResize
                                     | ImGuiWindowFlags_NoMove
                                     | ImGuiWindowFlags_NoBringToFrontOnFocus
                                     | ImGuiWindowFlags_NoNavFocus
                                     | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f, 0.0f));
        ImGui::Begin("##dockhost", nullptr, flags);
        ImGui::PopStyleVar(3);

        // El id se hashea contra el host, asi que solo es valido con el host abierto.
        return static_cast<u32>(ImGui::GetID("EfDockspace"));
    }

    void DebugUI::EndDockspace(u32 dockspaceId) {
        ImGui::DockSpace(static_cast<ImGuiID>(dockspaceId), ImVec2(0.0f, 0.0f),
                         ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();
    }

    void DebugUI::Render() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    bool DebugUI::WantsMouse()    const { return ImGui::GetIO().WantCaptureMouse; }
    bool DebugUI::WantsKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

}
}
