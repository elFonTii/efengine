#include "efengine/renderer/FxaaPass.h"
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/core/Assert.h>

#include <efecom/RHI.h>
#include <efengine/renderer/GpuProfiler.h>

namespace efengine {
namespace renderer {
    FxaaPass::FxaaPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* fxaaShader)
        : m_renderer(renderer)
        , m_quad(fullscreenQuad)
        , m_shader(fxaaShader) {
        EF_ASSERT(m_shader != null, "FxaaPass::FxaaPass: Se intenta inyectar shader nulo");
    }

    void FxaaPass::Resize(u32 width, u32 height) {} 

    void FxaaPass::Apply(const Texture& input, const RenderTarget& target) {
    EF_PROFILE_SCOPE("FXAA");
        target.Bind();

        const PostParamsBlock params {
            glm::vec4(m_settings.enabled ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f) };
        m_paramsUbo.Update(&params, sizeof(params));
        m_paramsUbo.BindTo(kPassBinding);

        m_shader->Bind();
        input.Bind(0);
        m_renderer.Draw(m_quad, *m_shader);
    }
}
}
