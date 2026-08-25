#pragma once
#include <vector>

namespace sandbox {

    struct EditorContext;

    // El panel de un pase. Puntero a funcion y no std::function: no captura
    // nada, porque cada panel encuentra su pase con
    // ctx.app.GetPipeline().Find<T>().
    //
    // Cada funcion abre su PROPIO CollapsingHeader: los titulos y los flags
    // (Sombras va con DefaultOpen) son parte del panel, no de la lista.
    using DrawPanelFn = void (*)(EditorContext&);

    // EL ORDEN DE LOS PANELES en la ventana de Render. Lista literal, igual que
    // BuildFramePipeline: agregar un pase es escribir su panel y sumarlo aca.
    const std::vector<DrawPanelFn>& PanelesDePases();

    // Los paneles que la lista referencia. Uno por archivo en panels/.
    void dibujarPanelSombras(EditorContext& ctx);
    void dibujarPanelDdgi(EditorContext& ctx);
    void dibujarPanelAo(EditorContext& ctx);

}
