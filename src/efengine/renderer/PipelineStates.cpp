#include "efengine/renderer/PipelineStates.h"

namespace efengine {
namespace renderer {

    efecom::PipelineState OpaqueState() {
        efecom::PipelineState s;
        s.depthTest   = true;
        s.depthWrite  = true;
        s.depthFunc   = efecom::DepthFunc::Less;
        s.cullMode    = efecom::CullMode::Back;
        s.blendEnable = false;
        return s;
    }

    efecom::PipelineState OpaqueDoubleSidedState() {
        efecom::PipelineState s = OpaqueState();
        s.cullMode = efecom::CullMode::None;
        return s;
    }

    // Los dos estados del forward cuando el depth prepass ya corrio.
    //
    // GL_EQUAL en vez de GL_LESS, y sin escribir profundidad: el prepass ya dejo
    // en el buffer la profundidad del fragmento VISIBLE de cada pixel, asi que
    // todo lo que no sea exactamente esa profundidad esta tapado y se descarta
    // ANTES del fragment shader. El overdraw se paga una sola vez, en el prepass,
    // que corre con un shader de dos lineas.
    //
    // Esto solo funciona si el prepass y el forward calculan gl_Position con
    // EXACTAMENTE los mismos bits. Por eso ao/depth_normal.vert y pbr.vert usan
    // la misma expresion y los dos declaran `invariant gl_Position`: sin esa
    // garantia el compilador puede reasociar la cadena de matrices en uno y no
    // en el otro, la igualdad falla por un ULP y la geometria desaparece a
    // parches. Es el modo de fallar clasico del depth prepass, y es total: no se
    // ve "un poco peor", no se ve nada.
    //
    // El culling se mantiene por material (el par doubleSided) porque el prepass
    // tambien lo hizo: si el forward culleara distinto, un triangulo que el
    // prepass no dibujo no tendria su profundidad en el buffer.
    efecom::PipelineState OpaqueEqualState() {
        efecom::PipelineState s = OpaqueState();
        s.depthFunc  = efecom::DepthFunc::Equal;
        s.depthWrite = false;
        return s;
    }

    efecom::PipelineState OpaqueDoubleSidedEqualState() {
        efecom::PipelineState s = OpaqueEqualState();
        s.cullMode = efecom::CullMode::None;
        return s;
    }

    // El skybox se dibuja PRIMERO y no escribe profundidad, pero SI la testea.
    //
    // skybox.vert emite z = w, o sea profundidad NDC 1.0: el plano lejano.
    // Con LessEqual eso pasa donde el buffer sigue en el 1.0 del clear (el
    // fondo) y falla donde la geometria ya escribio algo mas cerca. Con el depth
    // prepass corriendo antes, "algo mas cerca" es toda la escena, y el cielo
    // deja de sombrear los pixeles que la geometria va a tapar.
    //
    // Sin prepass el comportamiento es el de siempre: el clear deja 1.0 en todos
    // lados y el cielo pinta la pantalla entera.
    efecom::PipelineState SkyboxState() {
        efecom::PipelineState s;
        s.depthTest   = true;
        s.depthWrite  = false;
        s.depthFunc   = efecom::DepthFunc::LessEqual;
        s.cullMode    = efecom::CullMode::None;
        s.blendEnable = false;
        return s;
    }

    // Sin culling a proposito. Las paredes de un modelo de habitacion son de UNA
    // sola cara mirando hacia adentro: desde el sol se ve su cara trasera, y con
    // CullMode::Back se descartaban sin escribir profundidad -- el sol atravesaba
    // las paredes y el interior quedaba iluminado. El doubleSided del material no
    // alcanza: este pase aplica un solo PipelineState para todo y nunca mira los
    // materiales.
    //
    // El costo es que la cara de atras de cada objeto tambien se rasteriza, y que
    // un objeto sin espesor puede auto-sombrearse; para eso esta el bias
    // slope-scaled de ShadowFactor.
    efecom::PipelineState ShadowDepthState() {
        efecom::PipelineState s;
        s.depthTest   = true;
        s.depthWrite  = true;
        s.depthFunc   = efecom::DepthFunc::Less;
        s.cullMode    = efecom::CullMode::None;
        s.blendEnable = false;
        return s;
    }

    // Quad fullscreen de post: no participa de la profundidad de nada.
    efecom::PipelineState FullscreenState() {
        efecom::PipelineState s;
        s.depthTest   = false;
        s.depthWrite  = false;
        s.cullMode    = efecom::CullMode::None;
        s.blendEnable = false;
        return s;
    }

    // Sin culling a proposito, por la misma razon que ShadowDepthState pero al
    // reves: aca lo que importa NO es que la luz atraviese las paredes, sino que
    // un probe metido adentro de un solido VEA la cara interna que lo contiene.
    // Con CullMode::Back esa cara se descarta y el probe ve derecho a traves,
    // hacia el exterior, guarda distancias largas y radiancia brillante, y
    // Chebyshev concluye que la region es visible. Eso ERA el light leaking.
    //
    // El costo es que se rasteriza la cara de atras de cada objeto en las 6 caras
    // de cada probe. Se mide en el panel ("Pase (CPU)") antes y despues.
    efecom::PipelineState DdgiCaptureState() {
        efecom::PipelineState s = OpaqueState();
        s.cullMode = efecom::CullMode::None;
        return s;
    }

}
}
