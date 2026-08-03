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

    // El skybox se dibuja PRIMERO, con el depth buffer recien limpiado y sin
    // testear ni escribir profundidad: pinta el fondo entero y la geometria de
    // la escena lo tapa despues.
    efecom::PipelineState SkyboxState() {
        efecom::PipelineState s;
        s.depthTest   = false;
        s.depthWrite  = false;
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
