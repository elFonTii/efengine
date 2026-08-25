#include "efengine/renderer/ShadowPass.h"

#include <efecom/RHI.h>

#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/ShadowMath.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/core/Assert.h>
#include <efengine/renderer/GpuProfiler.h>

namespace efengine {
namespace renderer {

    ShadowPass::ShadowPass(Renderer& renderer, Shader* depthShader, u32 resolution)
        : m_renderer(renderer), m_shader(depthShader), m_shadowMap(resolution) {
        EF_ASSERT(m_shader != null, "ShadowPass: shader de profundidad nulo (fallo al cargar)");
        // Si no, un ShadowPass construido con otra resolucion se recrearia solo
        // en el primer Render para volver al default de ShadowSettings.
        m_settings.resolution = resolution;
    }

    void ShadowPass::Execute(FrameContext& ctx) {
        // El scope del profiler lo abre ahora el ScenePipeline con Name().
        const scene::SceneGraph& scene = ctx.scene;
        const DirectionalLight&  sun   = ctx.scene.Sun();

        // El encuadre sale de la escena, no de sliders. Antes el centro era un
        // glm::vec3(0) fijo, asi que una sala que no estuviera en el origen se
        // salia sola de la caja ortografica.
        //
        // WorldBounds() lo recalcula UpdateWorldTransforms, que Application ya
        // corre antes de este pase.
        m_fit = FitDirectionalLight(sun.direction, scene.WorldBounds(), m_settings.padding);

        // Recrear el FBO es caro, asi que solo cuando el valor cambio de verdad.
        if (m_settings.resolution != m_shadowMap.resolution() && m_settings.resolution > 0u) {
            m_shadowMap = ShadowMap(m_settings.resolution);
        }

        m_shadowMap.Bind();
        efecom::ApplyPipelineState(ShadowDepthState());
        efecom::Clear(efecom::ClearMask::Depth);

        // Una sola subida por pase: la matriz light-space no cambia entre objetos.
        const ShadowPassBlock pass { m_fit.matrix };
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

        // Lo que antes armaba Application con el valor de retorno de Render.
        ctx.lighting.shadow.map              = &m_shadowMap.DepthTexture();
        ctx.lighting.shadow.lightSpaceMatrix = m_fit.matrix;
        ctx.lighting.shadow.enabled          = true;
        ctx.lighting.shadow.biasMin          = m_settings.biasMin;
        ctx.lighting.shadow.biasMax          = m_settings.biasMax;

        // El normal offset se dial en texels pero viaja en metros: el shader no
        // sabe cuanto mide un texel del shadow map en el mundo, y eso depende
        // del encuadre que este pase acaba de calcular.
        const f32 texel = 2.0f * m_fit.orthoHalfSize
                        / static_cast<f32>(m_shadowMap.resolution());
        ctx.lighting.shadow.normalOffset = m_settings.normalOffsetTexels * texel;
    }

}
}
