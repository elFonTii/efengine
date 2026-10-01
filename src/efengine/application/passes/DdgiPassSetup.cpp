#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/DdgiPass.h>
#include <efengine/renderer/ShadowPass.h>

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
        shaders.probeUpdate = d.resources.GetComputeShader("ddgi_probe_update",
                              "assets/shaders/ddgi/probe_update.comp");
        shaders.schedule = d.resources.GetComputeShader("ddgi_schedule",
                              "assets/shaders/ddgi/schedule.comp");

        // Si falta cualquier shader, el pase no se registra y el frame sigue
        // con IBL puro: un fallo de carga no rompe el render.
        std::unique_ptr<renderer::DdgiPass> pase =
            renderer::DdgiPass::Create(d.renderer, d.fullscreenQuad, shaders);
        if (!pase) {
            EF_LOG_ERROR("DdgiPass: no se pudo crear");
            return;
        }

        // El mapa de escena del ShadowPass (registrado antes) sigue a DDGI.
        if (renderer::ShadowPass* sombra = pipeline.Find<renderer::ShadowPass>()) {
            sombra->FollowDdgi(&pase->settings());
        }
        pipeline.Add(std::move(pase));
    }

}
}
