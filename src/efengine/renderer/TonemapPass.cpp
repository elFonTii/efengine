#include "efengine/renderer/TonemapPass.h"

#include <efecom/RHI.h>
#include <efengine/core/Assert.h>
#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/PostTargets.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Texture.h>
#include <efengine/scene/Camera.h>

namespace efengine {
namespace renderer {

    TonemapPass::TonemapPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* shader)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_shader(shader) {
        EF_ASSERT(m_shader != null, "TonemapPass: shader nulo");
    }

    void TonemapPass::Execute(FrameContext& ctx) {
        efecom::ApplyPipelineState(FullscreenState());
        const Texture& input = ctx.post.Current();
        const RenderTarget target = ctx.post.AcquireNext();
        target.Bind();

        const PostParamsBlock p { glm::vec4(ctx.camera.Exposure(), 0.0f, 0.0f, 0.0f) };
        m_paramsUbo.Update(&p, sizeof(p));
        m_paramsUbo.BindTo(kPassBinding);

        m_shader->Bind();
        input.Bind(0);
        m_renderer.Draw(m_quad, *m_shader);
        ctx.post.Publish();
    }

}
}
