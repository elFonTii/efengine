#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/ShadowPass.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterShadowPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        // El ctor assertea si el shader es null: un shadow map sin shader de
        // profundidad no tiene degradacion posible.
        auto pase = std::make_unique<renderer::ShadowPass>(
            d.renderer,
            d.resources.GetShader("shadow_depth",
                "assets/shaders/shadow_depth.vert",
                "assets/shaders/shadow_depth.frag"));
        // Su unico consumidor es la captura de DDGI: el pase la sigue (lo
        // engancha RegisterDdgiPass) y sin DDGI no dibuja.
        //
        // DEUDA TECNICA: quedan dos caminos de sombra vivos, este encuadrado a la
        // escena y las cascadas encuadradas a la camara.
        pipeline.Add(std::move(pase));
    }

}
}
