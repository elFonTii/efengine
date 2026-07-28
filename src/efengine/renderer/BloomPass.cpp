#include "efengine/renderer/BloomPass.h"
#include <efengine/core/Assert.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Texture.h>
#include <efecom/RHI.h>
#include <utility>

#include <algorithm>

namespace efengine {
namespace renderer {
    BloomPass::BloomPass(Renderer& renderer, VertexArray& fullscreenQuad,
                      Shader* brightpass, Shader* blur, Shader* composite,
                      u32 width, u32 height)
                    : m_renderer(renderer), m_quad(fullscreenQuad)
                    , m_brightpass(brightpass), m_blur(blur), m_composite(composite)
                    , m_fboA(std::max(1u, width/2), std::max(1u, height/2))
                    , m_fboB(std::max(1u, width/2), std::max(1u, height/2)) {
                        EF_ASSERT(brightpass != null && blur != null && composite != null, "BloomPass::BloomPass: Se intenta inyectar shader nulo");
                    }

    void BloomPass::Resize(u32 width, u32 height) {
        m_fboA.Resize(std::max(1u,width/2), std::max(1u,height/2));
        m_fboB.Resize(std::max(1u,width/2), std::max(1u,height/2));
    }

void BloomPass::Apply(const Texture& input, const RenderTarget& target) {
    m_paramsUbo.BindTo(kPassBinding);

    // brightpass: escena full-res -> m_fboA (1/2 res)
    m_fboA.Bind();
    {
        const PostParamsBlock p {
            glm::vec4(m_settings.threshold, m_settings.knee, 0.0f, 0.0f) };
        m_paramsUbo.Update(&p, sizeof(p));
    }
    m_brightpass->Bind();
    input.Bind(0);
    m_renderer.Draw(m_quad, *m_brightpass);

    // ── 2) Blur separable ping-pong: 2*N pasadas
    Framebuffer* src = &m_fboA;
    Framebuffer* dst = &m_fboB;
    bool horizontal = true;
    const i32 passes = 2 * m_settings.iterations;

    for (i32 i = 0; i < passes; ++i) {
        dst->Bind();
        {
            const PostParamsBlock p {
                glm::vec4(horizontal ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f) };
            m_paramsUbo.Update(&p, sizeof(p));
        }
        m_blur->Bind();
        src->ColorTexture().Bind(0);
        m_renderer.Draw(m_quad, *m_blur);

        std::swap(src, dst);
        horizontal = !horizontal;
    }

    // Composite aditivo: escena full-res + blur (1/2 res, upscale LINEAR) sale al target
    target.Bind();
    {
        const PostParamsBlock p { glm::vec4(m_settings.intensity, 0.0f, 0.0f, 0.0f) };
        m_paramsUbo.Update(&p, sizeof(p));
    }
    m_composite->Bind();
    input.Bind(0);
    src->ColorTexture().Bind(1);
    m_renderer.Draw(m_quad, *m_composite);
}
}
}
