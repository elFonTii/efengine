#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/DdgiPass.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterDdgiPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        renderer::DdgiPass::Shaders shaders;
        shaders.trace = d.resources.GetComputeShader("ddgi_trace_voxel",
                              "assets/shaders/ddgi/trace_voxel.comp");
        shaders.voxelize = d.resources.GetShader("voxelize",
                              "assets/shaders/voxel/voxelize.vert",
                              "assets/shaders/voxel/voxelize.frag");
        shaders.blendIrradiance = d.resources.GetComputeShader("ddgi_blend_irradiance",
                              "assets/shaders/ddgi/blend_irradiance.comp");
        shaders.blendDistance = d.resources.GetComputeShader("ddgi_blend_distance",
                              "assets/shaders/ddgi/blend_distance.comp");

        // Si falta cualquier shader, el pase no se registra y el frame sigue
        // con IBL puro: un fallo de carga no rompe el render.
        if (!pipeline.Add(renderer::DdgiPass::Create(d.renderer, d.fullscreenQuad, shaders))) {
            EF_LOG_ERROR("DdgiPass: no se pudo crear");
        }
    }

}
}
