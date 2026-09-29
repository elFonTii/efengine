#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/AoPass.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterAoPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        renderer::AoPass::Shaders shaders;
        shaders.gtao = d.resources.GetShader("ao_gtao",
                                  "assets/shaders/screen.vert", "assets/shaders/ao/gtao.frag");
        shaders.denoise = d.resources.GetShader("ao_denoise",
                                  "assets/shaders/screen.vert", "assets/shaders/ao/denoise.frag");

        // Si falta un shader, el pase no se registra y el frame sigue sin
        // oclusion de contacto.
        if (!pipeline.Add(renderer::AoPass::Create(d.renderer, d.fullscreenQuad, shaders,
                                                   d.width, d.height))) {
            EF_LOG_ERROR("AoPass: no se pudo crear");
        }
    }

}
}
