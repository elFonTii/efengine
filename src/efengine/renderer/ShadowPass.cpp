#include "efengine/renderer/ShadowPass.h"

#include <efecom/RHI.h>

#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/ShadowMath.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    ShadowPass::ShadowPass(Renderer& renderer, Shader* depthShader, u32 resolution)
        : m_renderer(renderer), m_shader(depthShader), m_shadowMap(resolution) {
        EF_ASSERT(m_shader != null, "ShadowPass: shader de profundidad nulo (fallo al cargar)");
    }

    const glm::mat4& ShadowPass::Render(const scene::SceneGraph& scene, const DirectionalLight& sun) {
        m_lightSpaceMatrix = ComputeDirectionalLightMatrix(
            sun.direction, glm::vec3(0.0f),
            m_settings.orthoHalfSize, m_settings.distance,
            m_settings.nearPlane, m_settings.farPlane);

        m_shadowMap.Bind();
        efecom::SetDepthTest(true);
        efecom::Clear(efecom::ClearMask::Depth);

        // Dibujar la geometría de cada objeto (solo posición). Se re-bindea el
        // shader por objeto: Renderer::Draw desbindea el programa al terminar, y
        // los valores de uniform persisten en el programa entre binds.
        for (const scene::RenderItem& item : scene.Renderables()) {
            if (!item.model) continue;
            m_shader->Bind();
            m_shader->SetMat4("uLightSpaceMatrix", m_lightSpaceMatrix);
            m_shader->SetMat4("uModel", item.world);
            for (const Mesh& mesh : item.model->meshes()) {
                m_renderer.Draw(mesh.vertexArray(), *m_shader);
            }
        }

        return m_lightSpaceMatrix;
    }

}
}
