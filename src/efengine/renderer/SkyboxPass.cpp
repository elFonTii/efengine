#include "efengine/renderer/SkyboxPass.h"

#include <efecom/RHI.h>

#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/core/Assert.h>
#include <efengine/renderer/GpuProfiler.h>

namespace efengine {
namespace renderer {

    SkyboxPass::SkyboxPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* skyboxShader)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_shader(skyboxShader) {
        EF_ASSERT(m_shader != null, "SkyboxPass: shader de skybox nulo (fallo al cargar)");
    }

    void SkyboxPass::Draw(const Cubemap& env) const {
    EF_PROFILE_SCOPE("Skybox");
        // No se restaura nada al salir: el pase que sigue declara su propio estado.
        efecom::ApplyPipelineState(SkyboxState());

        m_shader->Bind();
        env.Bind(0);
        m_renderer.Draw(m_quad, *m_shader);
    }

}
}