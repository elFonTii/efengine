#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/Environment.h>
#include <efengine/renderer/IblPass.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterIblPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        renderer::Shader* eqCS = d.resources.GetComputeShader("equirect_to_cube",
                                     "assets/shaders/ibl/equirect_to_cube.comp");
        renderer::Shader* irrCS = d.resources.GetComputeShader("irradiance_convolve",
                                     "assets/shaders/ibl/irradiance_convolve.comp");
        renderer::Shader* preCS = d.resources.GetComputeShader("prefilter_ggx",
                                     "assets/shaders/ibl/prefilter_ggx.comp");
        renderer::Shader* lutCS = d.resources.GetComputeShader("brdf_lut",
                                     "assets/shaders/ibl/brdf_lut.comp");

        if (!eqCS || !irrCS || !preCS || !lutCS) {
            EF_LOG_ERROR("IblPass: no se pudo cargar algun compute de IBL "
                         "(equirect/irradiance/prefilter/brdf)");
            return;
        }

        renderer::EnvironmentDesc envDesc;
        envDesc.hdrPath = "assets/hdr/skybox.hdr";   // TODO: serializar en settings de escena

        renderer::EnvironmentShaders envShaders;
        envShaders.equirectToCube     = eqCS;
        envShaders.irradianceConvolve = irrCS;
        envShaders.prefilterGGX       = preCS;
        envShaders.brdfLut            = lutCS;

        // Si el precomputo falla, Create da null, el pase no se registra y el
        // shader apaga el ambiente entero en vez de muestrear basura.
        if (!pipeline.Add(renderer::IblPass::Create(
                renderer::Environment::Create(envDesc, envShaders)))) {
            EF_LOG_ERROR("IblPass: no se pudo crear el Environment IBL");
        }
    }

}
}
