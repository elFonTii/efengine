#include "efengine/renderer/FxaaPass.h"
#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/PostTargets.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/core/Assert.h>

#include <efecom/RHI.h>

namespace efengine {
namespace renderer {
    FxaaPass::FxaaPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* fxaaShader)
        : m_renderer(renderer)
        , m_quad(fullscreenQuad)
        , m_shader(fxaaShader) {
        EF_ASSERT(m_shader != null, "FxaaPass::FxaaPass: Se intenta inyectar shader nulo");
    }

    void FxaaPass::Execute(FrameContext& ctx) {
        efecom::ApplyPipelineState(FullscreenState());
        const Texture& input = ctx.post.Current();
        const RenderTarget target = ctx.post.AcquireNext();
        target.Bind();

        // params.x = 1: el pase apagado ahora es IScenePass::enabled.
        const PostParamsBlock params { glm::vec4(1.0f, 0.0f, 0.0f, 0.0f) };
        m_paramsUbo.Update(&params, sizeof(params));
        m_paramsUbo.BindTo(kPassBinding);

        m_shader->Bind();
        input.Bind(0);
        m_renderer.Draw(m_quad, *m_shader);
        ctx.post.Publish();
    }
}
}
