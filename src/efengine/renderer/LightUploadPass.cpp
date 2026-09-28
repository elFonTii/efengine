#include "efengine/renderer/LightUploadPass.h"

#include <efengine/core/Log.h>
#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/SceneGraph.h>

namespace efengine {
namespace renderer {

    void LightUploadPass::Execute(FrameContext& ctx) {
        const glm::mat4 vp = ctx.camera.ProjectionMatrix() * ctx.camera.ViewMatrix();
        PackLights(ctx.scene.Lights(), ExtractFrustum(vp), m_packed);

        m_stats.locals             = static_cast<u32>(m_packed.locals.size());
        m_stats.visible            = static_cast<u32>(m_packed.visible.size());
        m_stats.directional        = m_packed.block.counts.y;
        m_stats.droppedLocals      = m_packed.totalLocals - m_stats.locals;
        m_stats.droppedDirectional = m_packed.totalDirectional - m_stats.directional;

        // Una vez por cambio y no por frame: a 60 FPS un warning por frame tapa la consola.
        if (m_stats.droppedLocals != m_avisadoLocales) {
            if (m_stats.droppedLocals > 0u) {
                EF_LOG_WARNING("Luces: %u locales de mas (tope %u), se descartan",
                               m_stats.droppedLocals, kMaxLocalLights);
            }
            m_avisadoLocales = m_stats.droppedLocals;
        }
        if (m_stats.droppedDirectional != m_avisadoDireccionales) {
            if (m_stats.droppedDirectional > 0u) {
                EF_LOG_WARNING("Luces: %u direccionales de mas (tope %u), se descartan",
                               m_stats.droppedDirectional, kMaxDirectionalLights);
            }
            m_avisadoDireccionales = m_stats.droppedDirectional;
        }

        ctx.renderer.UploadLights(m_packed);
    }

}
}
