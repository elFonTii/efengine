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
        pipeline.Add(std::make_unique<renderer::ShadowPass>(
            d.renderer,
            d.resources.GetShader("shadow_depth",
                "assets/shaders/shadow_depth.vert",
                "assets/shaders/shadow_depth.frag")));
    }

}
}
