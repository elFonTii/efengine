#include "efengine/renderer/ShadowPass.h"

#include <glad/gl.h>

#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/ShadowMath.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/scene/Scene.h>
#include <efengine/scene/SceneObject.h>
#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    ShadowPass::ShadowPass(Renderer& renderer, Shader* depthShader, u32 resolution)
        : m_renderer(renderer), m_shader(depthShader), m_shadowMap(resolution) {
        EF_ASSERT(m_shader != null, "ShadowPass: shader de profundidad nulo (fallo al cargar)");
    }

    const glm::mat4& ShadowPass::Render(const scene::Scene& scene, const DirectionalLight& sun) {
        m_lightSpaceMatrix = ComputeDirectionalLightMatrix(
            sun.direction, glm::vec3(0.0f),
            m_settings.orthoHalfSize, m_settings.distance,
            m_settings.nearPlane, m_settings.farPlane);

        m_shadowMap.Bind();
        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Dibujar la geometría de cada objeto (solo posición). Se re-bindea el
        // shader por objeto: Renderer::Draw hace glUseProgram(0) al terminar, y
        // los valores de uniform persisten en el programa entre binds.
        for (const scene::SceneObject& obj : scene.objects()) {
            if (!obj.model) continue;
            m_shader->Bind();
            m_shader->SetMat4("uLightSpaceMatrix", m_lightSpaceMatrix);
            m_shader->SetMat4("uModel", obj.transform.Matrix());
            for (const Mesh& mesh : obj.model->meshes()) {
                m_renderer.Draw(mesh.vertexArray(), *m_shader);
            }
        }

        m_shadowMap.Unbind();
        return m_lightSpaceMatrix;
    }

}
}
