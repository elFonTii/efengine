#include "efengine/renderer/DdgiDebugPass.h"

#include <efecom/RHI.h>

#include <efengine/core/Assert.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/PipelineStates.h>

namespace efengine {
namespace renderer {

    DdgiDebugPass::DdgiDebugPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* blit)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_blit(blit) {
        EF_ASSERT(m_blit != null, "DdgiDebugPass: shader de blit nulo");
    }

    void DdgiDebugPass::DrawCaptureBlit(const Texture& captureTarget, bool showDistance) {
        // Sin depth: el recuadro va encima de todo. El discard del shader recorta
        // el resto del quad.
        efecom::ApplyPipelineState(FullscreenState());

        const PostParamsBlock params {
            glm::vec4(0.25f, showDistance ? 1.0f : 0.0f, 0.0f, 0.0f) };
        m_paramsUbo.Update(&params, sizeof(params));
        m_paramsUbo.BindTo(kPassBinding);

        m_blit->Bind();
        captureTarget.Bind(0);
        m_renderer.Draw(m_quad, *m_blit);
    }

}
}
