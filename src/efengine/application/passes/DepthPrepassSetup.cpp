#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/DepthPrepass.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>

namespace efengine {
namespace application {

    void RegisterDepthPrepass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        renderer::Shader* shader = d.resources.GetShader("ao_depth_normal",
                                       "assets/shaders/ao/depth_normal.vert",
                                       "assets/shaders/ao/depth_normal.frag");
        if (!pipeline.Add(renderer::DepthPrepass::Create(d.renderer, shader,
                                                         d.width, d.height, d.sceneFB))) {
            EF_LOG_ERROR("DepthPrepass: no se pudo crear; el forward resuelve la visibilidad solo");
        }
    }

}
}
