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

        // install_callbacks=true ENCADENA los callbacks GLFW que Window ya
        // registró en su ctor (DebugUI se construye después de m_window), así
        // ImGui recibe input y el CameraController sigue recibiendo el suyo.
        const bool okGlfw = ImGui_ImplGlfw_InitForOpenGL(window.GetNativeHandle(), true);
        EF_ASSERT(okGlfw, "DebugUI: fallo al iniciar el backend GLFW de ImGui");

        // "#version 330" = GLSL de OpenGL 3.3 Core, igual que el contexto de Window.
        const bool okGl = ImGui_ImplOpenGL3_Init("#version 330");
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

    void DebugUI::Render() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    bool DebugUI::WantsMouse()    const { return ImGui::GetIO().WantCaptureMouse; }
    bool DebugUI::WantsKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

} // namespace application
} // namespace efengine
