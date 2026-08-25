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
        // Los dos vertex shaders son propios del pase y ya no los de pbr/skybox:
        // la captura dibuja TODAS las vistas del frame con un draw instanciado,
        // y cada instancia saca su vista de un SSBO en vez del bloque Frame.
        shaders.capture = d.resources.GetShader("ddgi_capture",
                              "assets/shaders/ddgi/capture.vert",
                              "assets/shaders/ddgi/capture.frag");
        shaders.captureSky = d.resources.GetShader("ddgi_capture_sky",
                              "assets/shaders/ddgi/capture_sky.vert",
                              "assets/shaders/ddgi/capture_sky.frag");
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
