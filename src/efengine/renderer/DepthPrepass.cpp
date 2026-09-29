#include "efengine/renderer/DepthPrepass.h"

#include <efengine/core/Log.h>
#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/scene/SceneGraph.h>

namespace efengine {
namespace renderer {

    std::unique_ptr<DepthPrepass> DepthPrepass::Create(Renderer& renderer, Shader* depthNormal,
                                                       u32 width, u32 height, Framebuffer& sceneFB) {
        if (depthNormal == null) {
            EF_LOG_ERROR("DepthPrepass::Create: falta el shader depth_normal");
            return null;
        }
        if (width == 0u || height == 0u || !sceneFB.ownsDepth()) {
            EF_LOG_ERROR("DepthPrepass::Create: tamano %ux%u o sceneFB sin depth propio", width, height);
            return null;
        }
        return std::unique_ptr<DepthPrepass>(new DepthPrepass(renderer, depthNormal, width, height, sceneFB));
    }

    DepthPrepass::DepthPrepass(Renderer& renderer, Shader* shader, u32 width, u32 height,
                               Framebuffer& sceneFB)
        : m_renderer(renderer), m_shader(shader), m_sceneFB(&sceneFB)
        , m_normalFb(width, height, sceneFB.depthTextureId()) {}

    void DepthPrepass::Resize(u32 width, u32 height) {
        // El Resize de sceneFB ya corrio y recreo su depth: se lee el id ahora.
        if (width == 0u || height == 0u) return;
        m_normalFb.Resize(width, height, m_sceneFB->depthTextureId());
    }

    void DepthPrepass::Execute(FrameContext& ctx) {
        m_normalFb.Bind();
        // Alfa 0: viewZ == 0 es el centinela de "no hay geometria".
        m_renderer.Clear(0.0f, 0.0f, 0.0f, 0.0f);

        const AoPrepassBlock bloque { ctx.view.view, ctx.view.projection };
        m_ubo.Update(&bloque, sizeof(bloque));
        m_ubo.BindTo(kPassBinding);

        m_shader->Bind();
        for (const scene::RenderItem& item : ctx.scene.Renderables()) {
            if (!item.model) continue;
            // Shader sin estado forzado: cada material conserva su doubleSided.
            DrawOptions opciones;
            opciones.shader = m_shader;
            m_renderer.Submit(*item.model, *item.materials, item.world, opciones);
        }

        ctx.depthReady  = true;
        ctx.depthNormal = &m_normalFb.ColorTexture();
        ctx.depth       = &ctx.sceneFB.depthTexture();
    }

}
}
