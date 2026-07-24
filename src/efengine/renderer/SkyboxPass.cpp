#include "efengine/renderer/SkyboxPass.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_inverse.hpp>

#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    SkyboxPass::SkyboxPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* skyboxShader)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_shader(skyboxShader) {
        EF_ASSERT(m_shader != null, "SkyboxPass: shader de skybox nulo (fallo al cargar)");
    }

    void SkyboxPass::Draw(const Cubemap& env, const glm::mat4& view, const glm::mat4& projection) const {

        const glm::mat4 invVPRot = glm::inverse(projection * glm::mat4(glm::mat3(view)));

        glDisable(GL_DEPTH_TEST);

        m_shader->Bind();
        m_shader->SetMat4("uInvViewProjRot", invVPRot);
        env.Bind(0);
        m_renderer.Draw(m_quad, *m_shader);

        glEnable(GL_DEPTH_TEST);
    }

}
}