#include "efengine/application/FramePipeline.h"

#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/FrameUploadPass.h>

#include <memory>

namespace efengine {
namespace application {

    void RegisterFrameUploadPass(renderer::ScenePipeline& pipeline, const PassDeps&) {
        // Sin shaders ni targets: solo sube el bloque Frame.
        pipeline.Add(std::make_unique<renderer::FrameUploadPass>());
    }

}
}
