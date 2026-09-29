#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/BloomPass.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterBloomPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        renderer::Shader* bright    = d.resources.GetShader("brightpass",     "assets/shaders/screen.vert", "assets/shaders/brightpass.frag");
        renderer::Shader* blur      = d.resources.GetShader("blur",           "assets/shaders/screen.vert", "assets/shaders/blur.frag");
        renderer::Shader* composite = d.resources.GetShader("bloomcomposite", "assets/shaders/screen.vert", "assets/shaders/bloom_composite.frag");
        if (bright == null || blur == null || composite == null) {
            EF_LOG_ERROR("BloomPass: falta algun shader; el frame sigue sin bloom");
            return;
        }
        pipeline.Add(std::make_unique<renderer::BloomPass>(d.renderer, d.fullscreenQuad,
                                                           bright, blur, composite, d.width, d.height));
    }

}
}
