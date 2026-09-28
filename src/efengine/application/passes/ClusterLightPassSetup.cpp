#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ClusterLightPass.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterClusterLightPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        const renderer::Shader* cull = d.resources.GetComputeShader("lights_cluster_cull",
                                           "assets/shaders/lights/cluster_cull.comp");
        std::unique_ptr<renderer::ClusterLightPass> pase = renderer::ClusterLightPass::Create(cull);
        if (!pase) {
            EF_LOG_ERROR("ClusterLightPass: falta el shader de culling; pbr.frag recorre todas las visibles");
            return;
        }
        pipeline.Add(std::move(pase));
    }

}
}
