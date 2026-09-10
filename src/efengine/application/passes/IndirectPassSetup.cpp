#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/IndirectPass.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterIndirectPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        // Si falta el shader, el pase no se registra y pbr.frag samplea el
        // volumen inline: la misma imagen, mas cara.
        if (!pipeline.Add(renderer::IndirectPass::Create(
                d.renderer, d.fullscreenQuad,
                d.resources.GetShader("ddgi_indirect",
                                      "assets/shaders/screen.vert",
                                      "assets/shaders/ddgi/indirect.frag"),
                d.width, d.height))) {
            EF_LOG_ERROR("IndirectPass: no se pudo crear");
        }
    }

}
}
