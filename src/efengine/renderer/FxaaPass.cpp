#include "efengine/renderer/FxaaPass.h"
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/core/Assert.h>

#include <glad/gl.h>

namespace efengine {
namespace renderer {
    FxaaPass::FxaaPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* fxaaShader)
        : m_renderer(renderer)
        , m_quad(fullscreenQuad)
        , m_shader(fxaaShader) {
        EF_ASSERT(m_shader != null, "FxaaPass::FxaaPass: Se intenta inyectar shader nulo");
    }

    void FxaaPass::Resize(u32 width, u32 height) {} 

    void FxaaPass::Apply(const Texture& input, const Framebuffer* target) {
        if (target != null) {
            target->Bind();
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            m_renderer.SetViewport(input.width(), input.height());
        }

        m_shader->Bind();
        input.Bind(0);
        m_shader->SetInt("uScreenTexture", 0);
        m_shader->SetInt("uEnabled", m_settings.enabled ? 1 : 0);
        m_renderer.Draw(m_quad, *m_shader);
    }
}
}
