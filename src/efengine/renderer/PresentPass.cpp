#include "efengine/renderer/PresentPass.h"

#include <efecom/RHI.h>
#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/renderer/PostTargets.h>
#include <efengine/renderer/RenderTarget.h>

namespace efengine {
namespace renderer {

    void PresentPass::Execute(FrameContext& ctx) {
        const Framebuffer* src = ctx.post.CurrentFramebuffer();
        const u32 srcId = (src != null) ? src->id() : ctx.sceneFB.id();
        efecom::BlitColorToPresent(srcId, ctx.width, ctx.height);
        RenderTarget::Present().Bind();
    }

}
}
