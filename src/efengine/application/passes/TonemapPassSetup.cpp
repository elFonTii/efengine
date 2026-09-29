#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/TonemapPass.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterTonemapPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        renderer::Shader* s = d.resources.GetShader("tonemap", "assets/shaders/screen.vert", "assets/shaders/tonemap.frag");
        if (s == null) {
            EF_LOG_ERROR("TonemapPass: falta el shader; la pantalla recibe HDR crudo");
            return;
        }
        pipeline.Add(std::make_unique<renderer::TonemapPass>(d.renderer, d.fullscreenQuad, s));
    }

}
}
