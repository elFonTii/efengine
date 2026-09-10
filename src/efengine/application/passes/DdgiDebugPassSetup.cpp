#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/DdgiDebugPass.h>
#include <efengine/renderer/DdgiPass.h>
#include <efengine/renderer/Model.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterDdgiDebugPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        renderer::Shader* blit = d.resources.GetShader("ddgi_debug_blit",
                "assets/shaders/screen.vert", "assets/shaders/ddgi/debug_blit.frag");
        renderer::Shader* probe = d.resources.GetShader("ddgi_debug_probe",
                "assets/shaders/ddgi/debug_probe.vert", "assets/shaders/ddgi/debug_probe.frag");

        if (!blit || !probe) {
            EF_LOG_ERROR("DdgiDebugPass: no se pudieron cargar sus shaders");
            return;
        }

        // La esfera del volcado de probes. Si no carga, el modo de esferas se
        // saltea; el resto del debug de DDGI sigue funcionando.
        const renderer::Model* sphere = d.resources.GetModel("assets/models/sphere.fbx");
        if (sphere != null) {
            // El .fbx no viene unitario; DrawProbes divide por esto para que
            // debugRadius sea de verdad un radio en metros.
            const glm::vec3 e = sphere->bounds().Extents();
            EF_LOG_INFO("DdgiDebugPass: sphere.fbx semi-extents (%.3f, %.3f, %.3f)",
                        e.x, e.y, e.z);
        }

        // El DdgiPass ya esta registrado: este pase va ultimo en la lista. Sin
        // el, Find da null y el debug simplemente no dibuja.
        pipeline.Add(std::make_unique<renderer::DdgiDebugPass>(
            d.renderer, d.fullscreenQuad, blit, probe,
            pipeline.Find<renderer::DdgiPass>(), sphere));
    }

}
}
