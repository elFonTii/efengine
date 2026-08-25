#pragma once
#include <efengine/core/Types.h>

#include <imgui.h>

namespace sandbox {

    // Helpers compartidos por los paneles. Vivian en el namespace anonimo de
    // EditorUI.cpp; al partir los paneles en archivos propios pasaron a un
    // header para no tener una copia por unidad de traduccion.

    // Colores de mensaje.
    inline const ImVec4 kColorError { 1.00f, 0.40f, 0.40f, 1.0f };
    inline const ImVec4 kColorAviso { 1.00f, 0.80f, 0.30f, 1.0f };
    inline const ImVec4 kColorOk    { 0.45f, 0.85f, 0.45f, 1.0f };

    // Ancho reservado para las etiquetas de los campos. Los widgets se estiran
    // hasta el borde del panel menos esto, asi que todos empiezan y terminan
    // alineados en vez de tener cada uno el largo que le toco.
    constexpr f32 kAnchoEtiqueta = 150.0f;

    // RAII sobre PushItemWidth: se abre una al principio de cada panel y todo lo
    // que se dibuje adentro queda alineado sin repetir la llamada por widget.
    //
    // OJO: la pila de item width es POR VENTANA, asi que el guard tiene que
    // morir ANTES del ImGui::End() de su panel. Si vive hasta el final de la
    // funcion, el Pop cae en la ventana de afuera y ImGui asserta con "Calling
    // PopItemWidth() too many times!". Por eso los paneles lo meten en un
    // bloque propio.
    struct CamposAlineados {
        explicit CamposAlineados(f32 anchoEtiqueta = kAnchoEtiqueta) {
            ImGui::PushItemWidth(-anchoEtiqueta);
        }
        ~CamposAlineados() { ImGui::PopItemWidth(); }

        CamposAlineados(const CamposAlineados&)            = delete;
        CamposAlineados& operator=(const CamposAlineados&) = delete;
    };

}
