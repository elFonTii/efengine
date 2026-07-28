#include "efengine/renderer/ShadowPass.h"

#include <efecom/RHI.h>

#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/ShadowMath.h>
#include <efengine/renderer/PipelineStates.h>
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
        efecom::ApplyPipelineState(ShadowDepthState());
        efecom::Clear(efecom::ClearMask::Depth);

        // Una sola subida por pase: la matriz light-space no cambia entre objetos.
        const ShadowPassBlock pass { m_lightSpaceMatrix };
        m_passUbo.Update(&pass, sizeof(pass));
        m_passUbo.BindTo(kPassBinding);

        m_shader->Bind();
        for (const scene::RenderItem& item : scene.Renderables()) {
            if (!item.model) continue;
            // Lo unico que cambia por objeto es el bloque Object (binding 2).
            m_renderer.SetObjectMatrix(item.world);
            for (const Mesh& mesh : item.model->meshes()) {
                m_renderer.Draw(mesh.vertexArray(), *m_shader);
            }
        }

        return m_lightSpaceMatrix;
    }

}
}
