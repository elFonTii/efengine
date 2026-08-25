#include "efengine/renderer/FrameUploadPass.h"

#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/SceneGraph.h>

namespace efengine {
namespace renderer {

    void FrameUploadPass::Execute(FrameContext& ctx) {
        ctx.renderer.BeginScene(ctx.camera.ViewMatrix(), ctx.camera.ProjectionMatrix(),
                                ctx.camera.Position(), ctx.scene.PointLights(),
                                ctx.scene.Sun(), ctx.lighting);
    }

}
}
