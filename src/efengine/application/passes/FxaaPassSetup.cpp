#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/FxaaPass.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterFxaaPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        renderer::Shader* s = d.resources.GetShader("fxaa", "assets/shaders/screen.vert", "assets/shaders/fxaa.frag");
        if (s == null) {
            EF_LOG_ERROR("FxaaPass: falta el shader; el frame sigue sin antialiasing");
            return;
        }
        pipeline.Add(std::make_unique<renderer::FxaaPass>(d.renderer, d.fullscreenQuad, s));
    }

}
}
