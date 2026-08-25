#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/SkyboxPass.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterSkyboxPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        pipeline.Add(std::make_unique<renderer::SkyboxPass>(
            d.renderer, d.fullscreenQuad,
            d.resources.GetShader("skybox", "assets/shaders/skybox.vert",
                                            "assets/shaders/skybox.frag")));
    }

}
}
