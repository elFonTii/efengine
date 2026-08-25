#include "panels/PassPanel.h"

namespace sandbox {

    // El orden en que aparecen las secciones de pases en la ventana de Render.
    //
    // No tiene por que coincidir con el orden del frame (BuildFramePipeline):
    // este es el orden en que conviene LEERLOS. Sombras primero porque es lo que
    // mas se toca; la indirecta va adentro de DDGI, que es de donde sale.
    const std::vector<DrawPanelFn>& PanelesDePases() {
        static const std::vector<DrawPanelFn> paneles {
            &dibujarPanelSombras,
            &dibujarPanelDdgi,
            &dibujarPanelAo,
        };
        return paneles;
    }

}
