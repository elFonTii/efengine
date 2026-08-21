#include "DebugUI.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <efengine/core/Assert.h>
#include <efengine/renderer/GpuProfiler.h>

namespace efengine {
namespace application {

namespace {

    // Tema propio del editor. El gris base es rgb(48,48,48) = 0.188 lineal-en-sRGB
    // (ImGui no hace conversion de espacio: son bytes/255 tal cual).
    //
    // La escala se arma alrededor de ese gris: lo que esta HUNDIDO (campos de
    // entrada, fondos de popup) va mas oscuro, y lo que esta LEVANTADO (botones,
    // headers, pestanas) va mas claro. Asi la jerarquia se lee sin bordes.
    constexpr ImVec4 gris(f32 v, f32 a = 1.0f) { return ImVec4(v, v, v, a); }

    // Un unico acento azulado para todo lo interactivo (checks, sliders, seleccion).
    // Monocromo salvo esto: el color marca "esto responde", no decora.
    constexpr ImVec4 kAcento      { 0.26f, 0.45f, 0.62f, 1.00f };
    constexpr ImVec4 kAcentoHover { 0.32f, 0.54f, 0.73f, 1.00f };
    constexpr ImVec4 kAcentoFuerte{ 0.38f, 0.63f, 0.85f, 1.00f };

    void AplicarTemaOscuro() {
        // Se parte del dark de stock para no dejar ningun ImGuiCol_ sin definir
        // cuando la version de ImGui agrega uno nuevo.
        ImGui::StyleColorsDark();

        ImGuiStyle& st = ImGui::GetStyle();
        ImVec4*     c  = st.Colors;

        c[ImGuiCol_Text]                  = gris(0.88f);
        c[ImGuiCol_TextDisabled]          = gris(0.50f);
        c[ImGuiCol_WindowBg]              = gris(0.188f);          // rgb(48,48,48)
        c[ImGuiCol_ChildBg]               = gris(0.188f, 0.00f);
        c[ImGuiCol_PopupBg]               = gris(0.145f, 0.98f);
        c[ImGuiCol_Border]                = gris(0.110f);
        c[ImGuiCol_BorderShadow]          = gris(0.000f, 0.00f);

        // Campos de entrada: hundidos respecto del fondo.
        c[ImGuiCol_FrameBg]               = gris(0.130f);
        c[ImGuiCol_FrameBgHovered]        = gris(0.180f);
        c[ImGuiCol_FrameBgActive]         = gris(0.225f);

        c[ImGuiCol_TitleBg]               = gris(0.120f);
        c[ImGuiCol_TitleBgActive]         = gris(0.155f);
        c[ImGuiCol_TitleBgCollapsed]      = gris(0.120f, 0.75f);
        c[ImGuiCol_MenuBarBg]             = gris(0.150f);

        c[ImGuiCol_ScrollbarBg]           = gris(0.130f, 0.60f);
        c[ImGuiCol_ScrollbarGrab]         = gris(0.300f);
        c[ImGuiCol_ScrollbarGrabHovered]  = gris(0.380f);
        c[ImGuiCol_ScrollbarGrabActive]   = gris(0.450f);

        c[ImGuiCol_CheckMark]             = kAcentoFuerte;
        c[ImGuiCol_SliderGrab]            = kAcento;
        c[ImGuiCol_SliderGrabActive]      = kAcentoFuerte;

        // Botones: levantados, grises. El acento queda para la seleccion.
        c[ImGuiCol_Button]                = gris(0.260f);
        c[ImGuiCol_ButtonHovered]         = gris(0.330f);
        c[ImGuiCol_ButtonActive]          = gris(0.400f);

        c[ImGuiCol_Header]                = gris(0.280f);
        c[ImGuiCol_HeaderHovered]         = kAcento;
        c[ImGuiCol_HeaderActive]          = kAcentoHover;

        c[ImGuiCol_Separator]             = gris(0.290f);
        c[ImGuiCol_SeparatorHovered]      = kAcento;
        c[ImGuiCol_SeparatorActive]       = kAcentoFuerte;

        c[ImGuiCol_ResizeGrip]            = gris(0.300f, 0.40f);
        c[ImGuiCol_ResizeGripHovered]     = kAcento;
        c[ImGuiCol_ResizeGripActive]      = kAcentoFuerte;

        c[ImGuiCol_Tab]                   = gris(0.150f);
        c[ImGuiCol_TabHovered]            = kAcentoHover;
        c[ImGuiCol_TabSelected]           = gris(0.250f);
        c[ImGuiCol_TabSelectedOverline]   = kAcentoFuerte;
        c[ImGuiCol_TabDimmed]             = gris(0.135f);
        c[ImGuiCol_TabDimmedSelected]     = gris(0.200f);
        c[ImGuiCol_TabDimmedSelectedOverline] = gris(0.300f);

        c[ImGuiCol_DockingPreview]        = ImVec4(kAcento.x, kAcento.y, kAcento.z, 0.70f);
        c[ImGuiCol_DockingEmptyBg]        = gris(0.120f);

        c[ImGuiCol_PlotLines]             = gris(0.65f);
        c[ImGuiCol_PlotLinesHovered]      = kAcentoFuerte;
        c[ImGuiCol_PlotHistogram]         = kAcento;
        c[ImGuiCol_PlotHistogramHovered]  = kAcentoFuerte;

        c[ImGuiCol_TableHeaderBg]         = gris(0.230f);
        c[ImGuiCol_TableBorderStrong]     = gris(0.290f);
        c[ImGuiCol_TableBorderLight]      = gris(0.230f);
        c[ImGuiCol_TableRowBg]            = gris(0.000f, 0.00f);
        c[ImGuiCol_TableRowBgAlt]         = gris(1.000f, 0.03f);

        c[ImGuiCol_TextSelectedBg]        = ImVec4(kAcento.x, kAcento.y, kAcento.z, 0.45f);
        c[ImGuiCol_NavCursor]             = kAcentoFuerte;
        c[ImGuiCol_ModalWindowDimBg]      = gris(0.020f, 0.60f);

        // -- Metrica ---------------------------------------------------------
        // Un poco mas de aire vertical que el default y esquinas suaves: con
        // paneles de 30+ controles seguidos, el respiro es lo que los hace
        // legibles.
        st.WindowPadding     = ImVec2(10.0f, 8.0f);
        st.FramePadding      = ImVec2(8.0f,  4.0f);
        st.CellPadding       = ImVec2(6.0f,  4.0f);
        st.ItemSpacing       = ImVec2(8.0f,  6.0f);
        st.ItemInnerSpacing  = ImVec2(6.0f,  4.0f);
        st.IndentSpacing     = 18.0f;
        st.ScrollbarSize     = 12.0f;
        st.GrabMinSize       = 10.0f;

        st.WindowBorderSize  = 1.0f;
        st.ChildBorderSize   = 1.0f;
        st.PopupBorderSize   = 1.0f;
        st.FrameBorderSize   = 0.0f;   // el contraste de FrameBg ya delimita el campo
        st.TabBorderSize     = 0.0f;

        st.WindowRounding    = 4.0f;
        st.ChildRounding     = 4.0f;
        st.FrameRounding     = 3.0f;
        st.PopupRounding     = 4.0f;
        st.ScrollbarRounding = 6.0f;
        st.GrabRounding      = 3.0f;
        st.TabRounding       = 4.0f;

        // Titulos y etiquetas de pestana centrados; el resto alineado a la
        // izquierda, que es donde el ojo barre una lista de propiedades.
        st.WindowTitleAlign  = ImVec2(0.5f, 0.5f);
        st.ButtonTextAlign   = ImVec2(0.5f, 0.5f);
        st.SelectableTextAlign = ImVec2(0.0f, 0.5f);
    }

} // namespace

    DebugUI::DebugUI(platform::Window& window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        AplicarTemaOscuro();

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
    EF_PROFILE_SCOPE("ImGui");
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    bool DebugUI::WantsMouse()    const { return ImGui::GetIO().WantCaptureMouse; }
    bool DebugUI::WantsKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

}
}
