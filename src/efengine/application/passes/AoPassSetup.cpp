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
        shaders.depthNormal = d.resources.GetShader("ao_depth_normal",
                                  "assets/shaders/ao/depth_normal.vert",
                                  "assets/shaders/ao/depth_normal.frag");
        shaders.gtao = d.resources.GetShader("ao_gtao",
                                  "assets/shaders/screen.vert", "assets/shaders/ao/gtao.frag");
        shaders.denoise = d.resources.GetShader("ao_denoise",
                                  "assets/shaders/screen.vert", "assets/shaders/ao/denoise.frag");

        // El prepass escribe en el depth DEL FRAMEBUFFER DE ESCENA. Es lo que
        // deja al forward dibujar despues con GL_EQUAL en vez de volver a
        // resolver la visibilidad que el prepass ya resolvio.
        //
        // Si falta un shader, el pase no se registra y el frame sigue sin
        // oclusion de contacto.
        if (!pipeline.Add(renderer::AoPass::Create(d.renderer, d.fullscreenQuad, shaders,
                                                   d.width, d.height, d.sceneFB))) {
            EF_LOG_ERROR("AoPass: no se pudo crear");
        }
    }

}
}
