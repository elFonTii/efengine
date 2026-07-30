#pragma once

namespace sandbox {

    struct EditorContext;

    // Arma la sala de prueba de DDGI desde cero: limpia la escena y los assets,
    // genera la sala y los dos bloques, crea los cuatro materiales planos y el
    // sol, y re-resuelve los handles del editor.
    //
    // Interior de 8 x 4 x 8 m con el piso en y=0 y una abertura de 3 x 2 m en la
    // pared -Z. Las medidas son las que dan los defaults de DdgiSettings: si
    // cambian aca, cambian alla.
    void BuildCornellScene(EditorContext& ctx);

}
