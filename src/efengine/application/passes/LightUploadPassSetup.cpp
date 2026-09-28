#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/renderer/LightUploadPass.h>
#include <efengine/renderer/ScenePipeline.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterLightUploadPass(renderer::ScenePipeline& pipeline, const PassDeps&) {
        pipeline.Add(std::make_unique<renderer::LightUploadPass>());
    }

}
}
