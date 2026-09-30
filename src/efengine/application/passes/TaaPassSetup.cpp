#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/renderer/TaaPass.h>
#include <efengine/resources/ResourceManager.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterTaaPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        renderer::Shader* s = d.resources.GetShader("taa", "assets/shaders/screen.vert", "assets/shaders/taa.frag");
        if (s == null) {
            EF_LOG_ERROR("TaaPass: falta el shader; el frame sigue sin antialiasing");
            return;
        }
        pipeline.Add(std::make_unique<renderer::TaaPass>(d.renderer, d.fullscreenQuad, s, d.width, d.height));
    }

}
}
