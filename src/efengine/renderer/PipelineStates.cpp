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

    efecom::PipelineState ShadowDepthState() {
        efecom::PipelineState s;
        s.depthTest   = true;
        s.depthWrite  = true;
        s.depthFunc   = efecom::DepthFunc::Less;
        s.cullMode    = efecom::CullMode::Back;
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

}
}
