#include "efengine/renderer/TonemapPass.h"
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/core/Assert.h>

#include <glad/gl.h>

namespace efengine {
namespace renderer {
    TonemapPass::TonemapPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* tonemapShader) 
        : m_renderer(renderer)
        , m_quad(fullscreenQuad)
        , m_shader(tonemapShader) {
        EF_ASSERT(m_shader != null, "TonemapPass::TonemapPass: Se intenta inyectar shader nulo");
    }

    void TonemapPass::SetExposure(f32 e){
        m_exposure = e;
    }

    void TonemapPass::Resize(u32 width, u32 height) {}
    
    void TonemapPass::Apply(const Texture& input, const Framebuffer* target){
        if(target != null) { target->Bind(); }
        if(target == null) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            m_renderer.SetViewport(input.width(), input.height());
        }

        m_shader->Bind();
        input.Bind(0);
        m_shader->SetInt("uScreenTexture", 0);
        m_shader->SetFloat("uExposure", m_exposure);
        m_renderer.Draw(m_quad, *m_shader);
    }
}
}