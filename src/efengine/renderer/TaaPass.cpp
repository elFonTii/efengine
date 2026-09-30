#include "efengine/renderer/TaaPass.h"

#include <efecom/RHI.h>
#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/PostTargets.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/TaaMath.h>
#include <efengine/renderer/Texture.h>

namespace efengine {
namespace renderer {

    TaaPass::TaaPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* shader, u32 width, u32 height)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_shader(shader)
        , m_history{ Framebuffer(width, height), Framebuffer(width, height) } {
        EF_ASSERT(m_shader != null, "TaaPass: shader nulo");
    }

    void TaaPass::Resize(u32 width, u32 height) {
        // ScenePipeline::Resize corre en todos los frames: solo cuenta un cambio real.
        if (width == 0u || height == 0u) return;
        if (width == m_history[0].width() && height == m_history[0].height()) return;
        m_history[0].Resize(width, height);
        m_history[1].Resize(width, height);
        m_resetPending = true;
    }

    void TaaPass::Execute(FrameContext& ctx) {
        if (ctx.velocity == null || ctx.depth == null) {
            if (!m_warnedMissing) {
                EF_LOG_WARNING("TaaPass: sin velocidad o profundidad del prepass; se saltea");
                m_warnedMissing = true;
            }
            return;
        }

        const bool reset = m_resetPending;
        m_resetPending = false;

        const Texture&     actual    = ctx.post.Current();
        const Framebuffer& lectura   = m_history[1u - m_write];
        Framebuffer&       escritura = m_history[m_write];

        efecom::ApplyPipelineState(FullscreenState());
        escritura.Bind();

        const TaaBlock bloque = MakeTaaBlock(ctx.view, m_settings, reset);
        m_ubo.Update(&bloque, sizeof(bloque));
        m_ubo.BindTo(kPassBinding);

        m_shader->Bind();
        actual.Bind(0);
        lectura.ColorTexture().Bind(1);
        ctx.velocity->Bind(2);
        ctx.depth->Bind(3);
        m_renderer.Draw(m_quad, *m_shader);

        ctx.post.PublishExternal(escritura);
        m_write = 1u - m_write;
    }

}
}
