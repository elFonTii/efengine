#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/ForwardPass.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterForwardPass(renderer::ScenePipeline& pipeline, const PassDeps&) {
        // Sin shaders propios: cada material trae el suyo y Renderer::Submit
        // los bindea.
        pipeline.Add(std::make_unique<renderer::ForwardPass>());
    }

}
}
