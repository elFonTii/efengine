#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/SceneTargetPass.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterSceneTargetPass(renderer::ScenePipeline& pipeline, const PassDeps& d) {
        pipeline.Add(std::make_unique<renderer::SceneTargetPass>(d.clearColor));
    }

}
}
