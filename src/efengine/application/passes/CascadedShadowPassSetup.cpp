#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/CascadedShadowPass.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterCascadedShadowPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        pipeline.Add(std::make_unique<renderer::CascadedShadowPass>(
            d.renderer,
            d.resources.GetShader("shadow_depth",
                "assets/shaders/shadow_depth.vert",
                "assets/shaders/shadow_depth.frag")));
    }

}
}
