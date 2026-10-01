#include "efengine/renderer/DdgiDebugPass.h"

#include <efengine/renderer/DdgiPass.h>
#include <efengine/renderer/FrameContext.h>

#include <efecom/RHI.h>

#include <efengine/core/Assert.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/StorageBuffer.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/DdgiVolume.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

namespace efengine {
namespace renderer {

    DdgiDebugPass::DdgiDebugPass(Renderer& renderer, VertexArray& fullscreenQuad,
                                 Shader* blit, Shader* probe,
                                 const DdgiPass* ddgi, const Model* sphere)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_blit(blit), m_probe(probe)
        , m_ddgi(ddgi), m_sphere(sphere) {
        EF_ASSERT(m_blit  != null, "DdgiDebugPass: shader de blit nulo");
        EF_ASSERT(m_probe != null, "DdgiDebugPass: shader de esfera de probe nulo");
    }

    void DdgiDebugPass::DrawRayBlit(const Texture& rays, bool showDistance, u32 rayCount,
                                    u32 probeCount) {
        // Sin depth: el recuadro va encima de todo. El discard del shader recorta
        // el resto del quad.
        efecom::ApplyPipelineState(FullscreenState());

        const PostParamsBlock params {
            glm::vec4(0.3f, showDistance ? 1.0f : 0.0f,
                      static_cast<f32>(rayCount)   / static_cast<f32>(kMaxRaysPerProbe),
                      static_cast<f32>(probeCount) / static_cast<f32>(kMaxProbesPerFrame)) };
        m_paramsUbo.Update(&params, sizeof(params));
        m_paramsUbo.BindTo(kPassBinding);

        m_blit->Bind();
        rays.Bind(0);
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

    void DdgiDebugPass::Execute(FrameContext& ctx) {
        if (m_ddgi == null || !m_ddgi->settings().debugProbes) return;
        m_ddgi->probeData().BindTo(kProbeDataBinding);

        const DdgiSettings& ds = m_ddgi->settings();
        // Modo 3 = el mismo volcado pero mirando el alfa: sin esto la distancia
        // capturada no es verificable desde el panel.
        if (ds.debugMode >= 2u) {
            const u32 rayos  = std::clamp(ds.raysPerProbe, kMinRaysPerProbe, kMaxRaysPerProbe);
            const u32 probes = std::min(std::min(ds.probeBudget, kMaxProbesPerFrame), ProbeCount(ds.grid));
            DrawRayBlit(m_ddgi->rayBuffer(), ds.debugMode == 3u, rayos, probes);
        } else if (m_sphere != null) {
            DrawProbes(ds.grid, ds, *m_sphere);
        }
    }

}
}
