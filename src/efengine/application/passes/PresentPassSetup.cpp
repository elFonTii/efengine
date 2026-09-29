#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/PresentPass.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterPresentPass(renderer::ScenePipeline& pipeline, const PassDeps&) {
        pipeline.Add(std::make_unique<renderer::PresentPass>());
    }

}
}
