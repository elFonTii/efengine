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

        // Cortes por pixel. NO baja de 4: con 2, el arco que devuelve cada corte
        // sobre una superficie plana va de 0.98 a 1.52 segun su orientacion, y
        // promediar dos muestras de esa distribucion deja un ruido pixel-a-pixel
        // de ~0.057 contra una señal de ~0.028 -- el grano es el doble que el AO
        // que se quiere ver, y como el ruido es fijo en espacio de PANTALLA, se
        // arrastra sobre las superficies cuando la camara se mueve. Con 4 cortes
        // ese ruido cae a ~0.004.
        i32 slices = 4;
        i32 steps  = 8;    // pasos por lado de cada corte

        // Techo del radio proyectado, en pixeles. Existe por el caso "camara
        // pegada a una pared", donde radius/viewZ explota.
        f32 maxScreenRadius = 128.0f;

        // El kernel y el blur corren a resolucion REDUCIDA (ver ReducedRes.h) y
        // pbr.frag sube el resultado con el mismo upsample bilateral que la
        // indirecta. El prepass sigue a resolucion completa: es barato (0.05 ms
        // medidos), la marcha del kernel quiere la profundidad fina, y ES la
        // guia del upsample -- bajarlo de resolucion seria quedarse sin contra
        // que comparar los pesos.
        //
        // El AO tolera muy bien media resolucion: la señal es de contacto, de
        // baja frecuencia comparada con el albedo, y el blur bilateral que ya
        // corria detras la suaviza igual. Queda como interruptor para poder
        // medir el delta y para comparar la calidad lado a lado.
        bool halfRes = true;

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
