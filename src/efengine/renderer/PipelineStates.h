#pragma once
#include <efecom/RHI.h>

namespace efengine {
namespace renderer {

    // Los estados de rasterizacion con nombre del motor. Solo datos: ningun
    // gl* se llama aca. Cada pase pide el suyo y lo aplica entero; asi no hay
    // "restaurar el default" que se rompa cuando el default cambie.

    efecom::PipelineState OpaqueState();
    efecom::PipelineState OpaqueDoubleSidedState();
    efecom::PipelineState SkyboxState();
    efecom::PipelineState ShadowDepthState();
    efecom::PipelineState FullscreenState();
    efecom::PipelineState DdgiCaptureState();

}
}
