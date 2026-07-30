#include "efengine/renderer/DdgiDebugPass.h"

#include <efecom/RHI.h>

#include <efengine/core/Assert.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/DdgiVolume.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>

#include <glm/gtc/matrix_transform.hpp>

namespace efengine {
namespace renderer {

    DdgiDebugPass::DdgiDebugPass(Renderer& renderer, VertexArray& fullscreenQuad,
                                 Shader* blit, Shader* probe)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_blit(blit), m_probe(probe) {
        EF_ASSERT(m_blit  != null, "DdgiDebugPass: shader de blit nulo");
        EF_ASSERT(m_probe != null, "DdgiDebugPass: shader de esfera de probe nulo");
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

    void DdgiDebugPass::DrawProbes(const DdgiGrid& grid, const DdgiSettings& settings,
                                   const Model& sphere) {
        // Con depth test: los probes se ocluyen contra la escena, que es lo que
        // los hace legibles como puntos en el espacio.
        efecom::ApplyPipelineState(OpaqueState());
        m_probe->Bind();

        // debugRadius es un radio en METROS, y el modelo no tiene por que venir
        // unitario (el sphere.fbx del repo no lo es). Sin dividir por su radio
        // propio, las esferas tapan la escena entera y el sintoma es un manchon
        // negro que crece con el spacing en vez de una grilla.
        // El radio propio es el mayor semi-extent, no AABB::Radius(): ese es el
        // radio de la esfera que ENVUELVE la caja, sqrt(3) veces mas grande.
        const glm::vec3 ext = sphere.bounds().Extents();
        const f32 radioModelo = sphere.bounds().Valid()
                                ? glm::max(ext.x, glm::max(ext.y, ext.z)) : 1.0f;
        const f32 escala = settings.debugRadius / ((radioModelo > 0.0001f) ? radioModelo : 1.0f);

        const u32 total = ProbeCount(grid);
        for (u32 i = 0u; i < total; ++i) {
            const glm::vec3 p = ProbeWorldPosition(grid, i);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), p);
            model = glm::scale(model, glm::vec3(escala));
            m_renderer.SetObjectMatrix(model);

            // El indice del probe por draw, en el PassParams que ya existe.
            const PostParamsBlock params {
                glm::vec4(static_cast<f32>(i),
                          (settings.debugMode == 1u) ? 1.0f : 0.0f, 0.0f, 0.0f) };
            m_paramsUbo.Update(&params, sizeof(params));
            m_paramsUbo.BindTo(kPassBinding);

            for (const Mesh& mesh : sphere.meshes()) {
                m_renderer.Draw(mesh.vertexArray(), *m_probe);
            }
        }
    }

}
}
