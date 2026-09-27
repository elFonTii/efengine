#include "efengine/renderer/CascadedShadowPass.h"

#include <efecom/RHI.h>

#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/ShadowMath.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/Cull.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/Camera.h>
#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    CascadedShadowPass::CascadedShadowPass(Renderer& renderer, Shader* depthShader)
        : m_renderer(renderer), m_shader(depthShader)
        , m_map(2048u, kMaxCascades) {
        EF_ASSERT(m_shader != null,
                  "CascadedShadowPass: shader de profundidad nulo (fallo al cargar)");
        // Igual que ShadowPass: si no, el mapa construido con otra resolucion se
        // recrearia solo en el primer Render para volver al default.
        m_settings.resolution = m_map.resolution();
    }

    void CascadedShadowPass::Execute(FrameContext& ctx) {
        const scene::SceneGraph& scene = ctx.scene;
        const scene::Camera&     cam   = ctx.camera;
        const DirectionalLight&  sun   = scene.Sun();

        const u32 count = glm::clamp(m_settings.count, 1u, kMaxCascades);

        // Recrear el FBO es caro: solo cuando el valor cambio de verdad.
        if (m_settings.resolution != m_map.resolution() && m_settings.resolution > 0u) {
            m_map = CascadedShadowMap(m_settings.resolution, kMaxCascades);
        }

        f32 fars[kMaxCascades] = {};
        ComputeCascadeSplits(cam.NearPlane(), m_settings.shadowDistance,
                             count, m_settings.lambda, fars);

        const glm::mat4 invView = glm::inverse(cam.ViewMatrix());
        // El aspect sale de la proyeccion y no de la ventana: es el que la camara
        // esta usando de verdad este frame.
        const glm::mat4 proj   = cam.ProjectionMatrix();
        const f32       aspect = proj[1][1] / proj[0][0];

        efecom::ApplyPipelineState(ShadowDepthState());
        m_shader->Bind();

        for (u32 i = 0; i < count; ++i) {
            const f32 cerca = (i == 0u) ? cam.NearPlane() : fars[i - 1u];
            m_context.fits[i] = FitCascade(sun.direction, invView, cam.Fov(), aspect,
                                           cerca, fars[i], m_map.resolution(),
                                           m_settings.lightExtension);

            m_map.BindLayer(i);
            efecom::Clear(efecom::ClearMask::Depth);

            const ShadowPassBlock pass { m_context.fits[i].matrix };
            m_passUbo.Update(&pass, sizeof(pass));
            m_passUbo.BindTo(kPassBinding);

            const AABB volumen = CascadeCullVolume(m_context.fits[i], sun.direction);
            CullAabb(scene.MeshSpans(), volumen, m_visibles[i]);

            // La unidad del culling es la SUBMALLA: un .fbx importado es un solo
            // item con miles de submallas, asi que cullear por item descarta cero.
            for (u32 indice : m_visibles[i]) {
                const MeshSpan&          span = scene.MeshSpans()[indice];
                const scene::RenderItem& item = scene.Renderables()[span.item];
                if (!item.model) continue;
                m_renderer.SetObjectMatrix(item.world);
                m_renderer.Draw(item.model->meshes()[span.mesh].vertexArray(), *m_shader);
            }
        }

        m_context.map                = &m_map;
        m_context.enabled            = true;
        m_context.count              = count;
        m_context.blendRatio         = m_settings.blendRatio;
        m_context.normalOffsetTexels = m_settings.normalOffsetTexels;
        m_context.debugView          = m_settings.debugView;
        ctx.lighting.cascades        = m_context;
    }

}
}
