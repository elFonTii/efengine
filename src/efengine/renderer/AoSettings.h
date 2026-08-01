#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    // Todo lo ajustable del AO, y exactamente lo que expone el panel de ImGui.
    // Espeja el rol de ShadowSettings y DdgiSettings.
    //
    // Nada de esto esta serializado: se tunea por ImGui y se pierde al cerrar,
    // igual que sombra, bloom y DDGI.
    struct AoSettings {
        bool enabled = true;

        // METROS. Tiene que quedar por DEBAJO del espaciado de probes de DDGI:
        // es la regla que evita el doble oscurecimiento. Lo que el AO ocluye es
        // exactamente lo que la grilla de probes no puede ver. Con la grilla de
        // Cornell (spacing 1.5/1.0/1.5, o sea 1.0 m en el eje mas apretado),
        // 0.5 m es la mitad de ese margen.
        f32 radius = 0.5f;

        // Exponente sobre la visibilidad. Es el UNICO control de fuerza: se
        // aplica en gtao.frag y no se repite en pbr.frag.
        f32 intensity = 1.0f;

        // Fraccion del radio en la que se desvanece una muestra lejana. Sin
        // esto, un objeto lejano alineado en pantalla ocluye a uno cercano.
        // Divisor en el shader: SanitizeAoSettings lo mantiene > 0.
        f32 thickness = 0.25f;

        i32 slices = 2;    // cortes por pixel
        i32 steps  = 8;    // pasos por lado de cada corte

        // Techo del radio proyectado, en pixeles. Existe por el caso "camara
        // pegada a una pared", donde radius/viewZ explota.
        f32 maxScreenRadius = 128.0f;

        bool bentNormal  = true;   // alimenta el lookup de DDGI y el de irradiancia IBL
        bool multiBounce = true;   // aproximacion de Jimenez: el AO se tiñe con el albedo
        bool blur        = true;

        // 0=off, 1=visibilidad, 2=bent normal, 3=normal del prepass, 4=viewZ.
        // Los modos 3 y 4 los escribe gtao.frag (es quien tiene el prepass
        // bindeado) y saltean el blur: borronear el contenido del prepass no
        // significa nada.
        u32 debugView = 0u;
    };

}
}
